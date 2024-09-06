#include "stdafx.hpp"

#include "orcl.hpp"

using namespace sm;
using namespace sm::db;
using namespace sm::db::detail::orcl;

struct CellInfo {
    void *value;
    ub4 length;
};

static bool isStringType(ub2 type) noexcept {
    return type == SQLT_STR || type == SQLT_AFC || type == SQLT_CHR;
}

static bool isNumberType(ub2 type) noexcept {
    return type == SQLT_NUM || type == SQLT_INT || type == SQLT_FLT;
}

static bool isBoolType(ub2 type) noexcept {
    if constexpr (kHasBoolType)
        return type == kBoolType;

    return false;
}

static bool getCellBoolean(const OraColumnInfo& column) noexcept {
    if (isBoolType(column.type))
        return column.value.bol;

    return column.value.text[0] != '0';
}

static CellInfo initCellValue(OraEnvironment& env, OraError error, OraColumnInfo& column) noexcept {
    if (isStringType(column.type)) {
        column.value.text = (oratext*)env.malloc(column.columnWidth + 1);
        return CellInfo { column.value.text, column.columnWidth + 1 };
    }

    switch (column.type) {
    case SQLT_VNU:
        return CellInfo { &column.value.num, sizeof(column.value.num) };

    case SQLT_DAT:
        return CellInfo { &column.value.date, sizeof(column.value.date) };

    case SQLT_INT:
        return CellInfo { &column.value.integer, sizeof(column.value.integer) };

    case SQLT_FLT:
        return CellInfo { &column.value.real, sizeof(column.value.real) };

    case kBoolType:
        return CellInfo { &column.value.bol, sizeof(column.value.bol) };

    default:
        CT_NEVER("Unsupported data type %d", column.type);
    }
}

static DbResult<ub2> getColumnSize(OraError error, OraParam param, bool charSemantics) noexcept {
    if (charSemantics) {
        return param.getAttribute<ub2>(error, OCI_ATTR_CHAR_SIZE);
    } else {
        return param.getAttribute<ub2>(error, OCI_ATTR_DATA_SIZE);
    }
}

static DbResult<std::string_view> getColumnName(OraError error, OraParam param) noexcept {
    ub4 columnNameLength = 0;
    text *columnNameBuffer = nullptr;
    if (sword status = param.getAttribute(error, OCI_ATTR_NAME, &columnNameBuffer, &columnNameLength))
        return std::unexpected(oraGetError(error, status));

    return std::string_view{(const char*)columnNameBuffer, columnNameLength};
}

static DbResult<ub2> getColumnType(OraError error, OraParam param) noexcept {
    ub2 dataType = 0;
    if (sword status = param.getAttribute(error, OCI_ATTR_DATA_TYPE, &dataType))
        return std::unexpected(oraGetError(error, status));

    if (dataType == SQLT_NUM)
        dataType = SQLT_VNU;

    return dataType;
}

static DbResult<std::vector<OraColumnInfo>> defineColumns(OraEnvironment& env, OraError error, OraStmt stmt) noexcept {
    OraParam param;
    std::vector<OraColumnInfo> columns;
    ub4 counter = 1;
    sword result = OCIParamGet(stmt, OCI_HTYPE_STMT, error, param.voidpp(), counter);
    if (result != OCI_SUCCESS)
        return columns;

    columns.reserve(16);

    while (result == OCI_SUCCESS) {
        ub2 dataType = TRY_RESULT(getColumnType(error, param));

        std::string_view columnName = TRY_RESULT(getColumnName(error, param));

        ub1 charSemantics = TRY_RESULT(param.getAttribute<ub1>(error, OCI_ATTR_CHAR_USED));

        ub2 columnWidth = TRY_RESULT(getColumnSize(error, param, charSemantics));

        auto& info = columns.emplace_back(OraColumnInfo {
            .name = columnName,
            .charSemantics = charSemantics,
            .columnWidth = columnWidth,
            .type = dataType,
        });

        if (isNumberType(dataType)) {
            info.precision = TRY_RESULT(param.getAttribute<sb2>(error, OCI_ATTR_PRECISION));
            info.scale = TRY_RESULT(param.getAttribute<sb1>(error, OCI_ATTR_SCALE));
        }

        auto [value, size] = initCellValue(env, error, info);

        sword status = OCIDefineByPos(
            stmt, &info.define, error, counter,
            value, size, dataType,
            nullptr, &info.valueLength, nullptr,
            OCI_DEFAULT
        );

        if (status != OCI_SUCCESS)
            return std::unexpected(oraGetError(error, status));

        result = OCIParamGet(stmt, OCI_HTYPE_STMT, error, param.voidpp(), ++counter);
    }

    return columns;
}

bool OraStatement::useBoolType() const noexcept {
    ub2 type = mConnection.getBoolType();
    return isBoolType(type);
}

DbError OraStatement::closeColumns() noexcept {
    for (OraColumnInfo& column : mColumnInfo)
        if (isStringType(column.type))
            mEnvironment.free(column.value.text);

    mColumnInfo.clear();
    return DbError::ok();
}

DbError OraStatement::execute(ub4 flags, int iters) noexcept {
    if (DbError error = closeColumns())
        return error;

    sword status = OCIStmtExecute(mConnection.service(), mStatement, mError, iters, 0, nullptr, nullptr, flags);

    bool shouldDefine = isSuccess(status);
    if (status == OCI_NO_DATA)
        status = OCI_SUCCESS;

    if (!isSuccess(status))
        return oraGetError(mError, status);

    if (shouldDefine)
        mColumnInfo = TRY_UNWRAP(defineColumns(mEnvironment, mError, mStatement));

    return DbError::ok();
}

DbError OraStatement::executeUpdate(ub4 flags) noexcept {
    return execute(flags, 1);
}

DbError OraStatement::executeSelect(ub4 flags) noexcept {
    return execute(flags, 0);
}

int OraStatement::getBindCount() const noexcept {
    return -1;
}

DbError OraStatement::bindAtPosition(int index, void *value, ub4 size, ub2 type) noexcept {
    OraBind bind;
    sword status = OCIBindByPos(
        mStatement, &bind, mError, index + 1,
        value, size, type,
        nullptr, nullptr, nullptr,
        0, nullptr, OCI_DEFAULT
    );

    if (status != OCI_SUCCESS)
        return oraGetError(mError, status);

    return DbError::ok();
}

DbError OraStatement::bindAtName(std::string_view name, void *value, ub4 size, ub2 type) noexcept {
    OraBind bind;
    sword status = OCIBindByName(
        mStatement, &bind, mError, (text*)name.data(), name.size(),
        value, size, type,
        nullptr, nullptr, nullptr,
        0, nullptr, OCI_DEFAULT
    );

    if (status != OCI_SUCCESS)
        return oraGetError(mError, status);

    return DbError::ok();
}

DbError OraStatement::close() noexcept {
    if (DbError error = closeColumns())
        return error;

    if (sword result = OCIStmtRelease(mStatement, mError, nullptr, 0, OCI_DEFAULT))
        return oraGetError(mError, result);

    DbError result = mError.close(nullptr);
    return oraGetError(mError, result);
}

DbError OraStatement::getColumnIndex(std::string_view name, int& index) const noexcept {
    for (int i = 0; i < mColumnInfo.size(); ++i) {
        if (mColumnInfo[i].name == name) {
            index = i;
            return DbError::ok();
        }
    }

    return DbError::columnNotFound(name);
}

DbError OraStatement::bindIntByIndex(int index, int64 value) noexcept {
    fmt::println(stderr, "bindIntByIndex: index={}, value={}", index, value);
    return bindAtPosition(index, &value, sizeof(value), SQLT_INT);
}

DbError OraStatement::bindBooleanByIndex(int index, bool value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];

    if (isBoolType(column.type))
        return bindAtPosition(index, &value, sizeof(value), kBoolType);

    return bindAtPosition(index, (void*)(value ? "1" : "0"), 1, SQLT_CHR);
}

DbError OraStatement::bindStringByIndex(int index, std::string_view value) noexcept {
    return bindAtPosition(index, (void*)value.data(), value.size(), SQLT_CHR);
}

DbError OraStatement::bindDoubleByIndex(int index, double value) noexcept {
    return bindAtPosition(index, &value, sizeof(value), SQLT_FLT);
}

DbError OraStatement::bindBlobByIndex(int index, Blob value) noexcept {
    return bindAtPosition(index, (void*)value.data(), value.size_bytes(), SQLT_BIN);
}

DbError OraStatement::bindNullByIndex(int index) noexcept {
    return bindAtPosition(index, nullptr, 0, SQLT_STR);
}

DbError OraStatement::bindIntByName(std::string_view name, int64 value) noexcept {
    fmt::println(stderr, "bindIntByName: name={}, value={}", name, value);

    // TODO: leaks
    return bindAtName(name, new int64{value}, sizeof(value), SQLT_INT);
}

DbError OraStatement::bindBooleanByName(std::string_view name, bool value) noexcept {
    int index = -1;
    if (DbError error = findBindIndex(name, index))
        return error;

    const OraColumnInfo& column = mColumnInfo[index];

    if (isBoolType(column.type))
        return bindAtName(name, &value, sizeof(value), kBoolType);

    return bindAtName(name, (void*)(value ? "1" : "0"), 1, SQLT_CHR);
}

DbError OraStatement::bindStringByName(std::string_view name, std::string_view value) noexcept {
    return bindAtName(name, (void*)value.data(), value.size(), SQLT_CHR);
}

DbError OraStatement::bindDoubleByName(std::string_view name, double value) noexcept {
    return bindAtName(name, &value, sizeof(value), SQLT_FLT);
}

DbError OraStatement::bindBlobByName(std::string_view name, Blob value) noexcept {
    return bindAtName(name, (void*)value.data(), value.size_bytes(), SQLT_BIN);
}

DbError OraStatement::bindNullByName(std::string_view name) noexcept {
    return bindAtName(name, nullptr, 0, SQLT_STR);
}

DbError OraStatement::select() noexcept {
    return executeSelect(OCI_DEFAULT);
}

DbError OraStatement::update(bool autoCommit) noexcept {
    ub4 flags = autoCommit ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT;
    return executeUpdate(flags);
}

DbError OraStatement::reset() noexcept {
    // TODO: some statements need to be reset, others dont
    return DbError::todo();
}

int OraStatement::getColumnCount() const noexcept {
    return mColumnInfo.size();
}

DbError OraStatement::next() noexcept {
    sword result = OCIStmtFetch2(mStatement, mError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
    return oraGetError(mError, result);
}

DbError OraStatement::getIntByIndex(int index, int64& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    const CellValue& cell = column.value;

    if (column.type == SQLT_VNU) {
        int64 integer;
        if (sword status = OCINumberToInt(mError, &cell.num, sizeof(integer), OCI_NUMBER_SIGNED, &integer))
            return oraGetError(mError, status);

        value = integer;
    } else {
        value = cell.integer;
    }

    return DbError::ok();
}

DbError OraStatement::getBooleanByIndex(int index, bool& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    value = getCellBoolean(column);
    return DbError::ok();
}

DbError OraStatement::getStringByIndex(int index, std::string_view& value) noexcept {
    const char *text = (const char*)mColumnInfo[index].value.text;
    value = {text, mColumnInfo[index].valueLength};
    return DbError::ok();
}

DbError OraStatement::getDoubleByIndex(int index, double& value) noexcept {
    value = mColumnInfo[index].value.real;
    return DbError::ok();
}

DbError OraStatement::getBlobByIndex(int index, Blob& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    const CellValue& cell = column.value;

    value = Blob{cell.text, cell.text + column.valueLength};
    return DbError::ok();
}
