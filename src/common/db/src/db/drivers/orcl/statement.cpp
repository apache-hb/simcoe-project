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

static bool isBlobType(ub2 type) noexcept {
    return type == SQLT_BLOB || type == SQLT_BIN;
}

static bool isNumberType(ub2 type) noexcept {
    return type == SQLT_NUM || type == SQLT_INT || type == SQLT_VNU;
}

static bool isBoolType(ub2 type) noexcept {
    return type == SQLT_BOL;
}

static bool getCellBoolean(const OraColumnInfo& column) noexcept {
    if (isBoolType(column.type))
        return column.value.bol;

    return column.value.text[0] != '0';
}

static CellInfo initCellValue(OraEnvironment& env, OraError error, OraColumnInfo& column) noexcept {
    if (isStringType(column.type)) {
        ub4 length = (column.columnWidth + 1);
        column.value.text = (char*)env.malloc(length);
        column.value.text[0] = '\0';
        return CellInfo { column.value.text, length };
    }

    if (isBlobType(column.type)) {
        column.value.lob = env.malloc(column.columnWidth);
        return CellInfo { column.value.lob, column.columnWidth };
    }

    switch (column.type) {
    case SQLT_VNU:
        OCINumberSetZero(error, &column.value.num);
        return CellInfo { &column.value.num, sizeof(column.value.num) };

    case SQLT_DAT:
        return CellInfo { &column.value.date, sizeof(column.value.date) };

    case SQLT_INT:
        return CellInfo { &column.value.integer, sizeof(column.value.integer) };

    case SQLT_BOL:
        return CellInfo { &column.value.bol, sizeof(column.value.bol) };

    case SQLT_FLT:
        return CellInfo { &column.value.real, sizeof(column.value.real) };

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
    if (sword status = param.getAttribute(error, OCI_ATTR_NAME, (void*)&columnNameBuffer, &columnNameLength))
        return std::unexpected(oraGetError(error, status));

    return std::string_view{(const char*)columnNameBuffer, columnNameLength};
}

static DbResult<ub2> getColumnType(OraError error, OraParam param) noexcept {
    return param.getAttribute<ub2>(error, OCI_ATTR_DATA_TYPE);
}

static DbResult<std::vector<OraColumnInfo>> defineColumns(OraEnvironment& env, OraError error, OraStmt stmt) noexcept {
    std::vector<OraColumnInfo> columns;
    ub4 params = TRY_RESULT(stmt.getAttribute<ub4>(error, OCI_ATTR_PARAM_COUNT));
    columns.reserve(params + 1);

    for (ub4 i = 1; i <= params; i++) {
        OraParam param;
        sword result = OCIParamGet(stmt, OCI_HTYPE_STMT, error, param.voidpp(), i);
        if (result != OCI_SUCCESS)
            return std::unexpected(oraGetError(error, result));

        ub2 dataType = TRY_RESULT(getColumnType(error, param));
        if (dataType == SQLT_NUM)
            dataType = SQLT_VNU;
        if (dataType == SQLT_CHR)
            dataType = SQLT_STR;
        if (dataType == SQLT_BLOB)
            dataType = SQLT_BIN;
        if (dataType == SQLT_IBFLOAT || dataType == SQLT_IBDOUBLE)
            dataType = SQLT_FLT;

        std::string_view columnName = TRY_RESULT(getColumnName(error, param));

        ub1 charSemantics = TRY_RESULT(param.getAttribute<ub1>(error, OCI_ATTR_CHAR_USED));

        ub2 columnWidth = TRY_RESULT(getColumnSize(error, param, charSemantics));

        auto& info = columns.emplace_back(OraColumnInfo {
            .name = columnName,
            .charSemantics = charSemantics,
            .columnWidth = columnWidth,
            .type = dataType,
        });

        if (dataType == SQLT_NUM) {
            info.precision = TRY_RESULT(param.getAttribute<sb2>(error, OCI_ATTR_PRECISION));
            info.scale = TRY_RESULT(param.getAttribute<sb1>(error, OCI_ATTR_SCALE));
        }

        auto [value, size] = initCellValue(env, error, info);

        info.valueLength = size;

        sword status = OCIDefineByPos2(
            stmt, &info.define, error, i,
            value, size, dataType,
            &info.indicator, &info.valueLength, nullptr,
            OCI_DEFAULT
        );

        if (status != OCI_SUCCESS)
            return std::unexpected(oraGetError(error, status));
    }

    return columns;
}

void *OraStatement::initBindValue(const void *value, ub4 size) noexcept {
    void *result = std::malloc(size);
    std::memcpy(result, value, size);
    mBindValues.push_back(result);
    return result;
}

void *OraStatement::initStringBindValue(std::string_view value) noexcept {
    void *result = mEnvironment.malloc(value.size() + 1);
    std::memcpy(result, value.data(), value.size());
    ((char*)result)[value.size()] = '\0';
    mBindValues.push_back(result);
    return result;
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

void OraStatement::freeBindValues() noexcept {
    for (void *value : mBindValues)
        mEnvironment.free(value);

    mBindValues.clear();
}

DbError OraStatement::executeStatement(ub4 flags, int iters) noexcept {
    if (DbError error = closeColumns())
        return error;

    sword status = OCIStmtExecute(mConnection.service(), mStatement, mError, iters, 0, nullptr, nullptr, flags);

    if (status == OCI_NO_DATA)
        status = OCI_SUCCESS;

    if (!isSuccess(status))
        return oraGetError(mError, status);

    mColumnInfo = TRY_UNWRAP(defineColumns(mEnvironment, mError, mStatement));

    return DbError::ok();
}

DbError OraStatement::executeUpdate(ub4 flags) noexcept {
    return executeStatement(flags, 1);
}

DbError OraStatement::executeSelect(ub4 flags) noexcept {
    return executeStatement(flags, 0);
}

int OraStatement::getBindCount() const noexcept {
    return mStatement.getAttribute<ub4>(mError, OCI_ATTR_BIND_COUNT).value_or(-1);
}

DbError OraStatement::getBindIndex(std::string_view name, int& index) const noexcept {
    return DbError::todo(fmt::format("getBindIndex({})", name));
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

DbError OraStatement::finalize() noexcept {
    freeBindValues();

    if (DbError error = closeColumns())
        return error;

    if (sword result = OCIStmtRelease(mStatement, mError, nullptr, 0, OCI_DEFAULT))
        return oraGetError(mError, result);

    DbError result = mError.close(nullptr);
    return oraGetError(mError, result);
}

DbError OraStatement::start(bool autoCommit, StatementType type) noexcept {
    bool isQuery = (type == StatementType::eQuery);
    int iters = isQuery ? 0 : 1;
    ub4 flags = autoCommit ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT;
    if (DbError error = executeStatement(flags, iters))
        return error;

    if (isQuery)
        return next();

    return DbError::ok();
}

DbError OraStatement::execute() noexcept {
    return DbError::ok();
}

DbError OraStatement::next() noexcept {
    sword result = OCIStmtFetch2(mStatement, mError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
    mHasData = (result == OCI_SUCCESS);
    return oraGetError(mError, result);
}

static std::string toUpper(std::string_view str) noexcept {
    std::string result;
    result.reserve(str.size());
    for (char c : str)
        result.push_back(std::toupper(c));

    return result;
}

DbError OraStatement::getColumnIndex(std::string_view name, int& index) const noexcept {
    std::string search = toUpper(name);
    for (int i = 0; i < mColumnInfo.size(); ++i) {
        if (mColumnInfo[i].name == search) {
            index = i;
            return DbError::ok();
        }
    }

    return DbError::columnNotFound(name);
}

DbError OraStatement::bindIntByIndex(int index, int64 value) noexcept {
    return bindAtPosition(index, &value, sizeof(value), SQLT_INT);
}

DbError OraStatement::bindBooleanByIndex(int index, bool value) noexcept {
    static constexpr bool kValues[] = {false, true};
    const bool *ptr = &kValues[value];
    return bindAtPosition(index, (void*)ptr, sizeof(value), SQLT_BOL);
}

DbError OraStatement::bindStringByIndex(int index, std::string_view value) noexcept {
    void *data = initStringBindValue(value);
    return bindAtPosition(index, data, value.size(), SQLT_CHR);
}

DbError OraStatement::bindDoubleByIndex(int index, double value) noexcept {
    return bindAtPosition(index, &value, sizeof(value), SQLT_FLT);
}

DbError OraStatement::bindBlobByIndex(int index, Blob value) noexcept {
    void *data = initBindValue(value.data(), value.size());
    return bindAtPosition(index, data, value.size(), SQLT_BIN);
}

DbError OraStatement::bindNullByIndex(int index) noexcept {
    return bindAtPosition(index, nullptr, 0, SQLT_STR);
}

DbError OraStatement::bindIntByName(std::string_view name, int64 value) noexcept {
    void *data = initBindValue(&value, sizeof(value));
    return bindAtName(name, data, sizeof(value), SQLT_INT);
}

DbError OraStatement::bindBooleanByName(std::string_view name, bool value) noexcept {
    static constexpr bool kValues[] = {false, true};
    const bool *ptr = &kValues[value];
    return bindAtName(name, (void*)ptr, sizeof(value), SQLT_BOL);
}

DbError OraStatement::bindStringByName(std::string_view name, std::string_view value) noexcept {
    void *data = initStringBindValue(value);
    return bindAtName(name, data, value.size(), SQLT_CHR);
}

DbError OraStatement::bindDoubleByName(std::string_view name, double value) noexcept {
    void *data = initBindValue(&value, sizeof(value));
    return bindAtName(name, data, sizeof(value), SQLT_FLT);
}

DbError OraStatement::bindBlobByName(std::string_view name, Blob value) noexcept {
    void *data = initBindValue(value.data(), value.size());
    return bindAtName(name, data, value.size(), SQLT_BIN);
}

DbError OraStatement::bindNullByName(std::string_view name) noexcept {
    return bindAtName(name, nullptr, 0, SQLT_STR);
}


DbError OraStatement::isNullByIndex(int index, bool& value) noexcept {
    value = mColumnInfo[index].indicator;
    return DbError::ok();
}

DbError OraStatement::isNullByName(std::string_view column, bool& value) noexcept {
    int index = -1;
    if (DbError error = getColumnIndex(column, index))
        return error;

    return isNullByIndex(index, value);
}

int OraStatement::getColumnCount() const noexcept {
    return mColumnInfo.size();
}

static DataType getColumnType(ub2 type) noexcept {
    if (isStringType(type))
        return DataType::eString;

    if (isBlobType(type))
        return DataType::eBlob;

    if (type == SQLT_FLT)
        return DataType::eDouble;

    if (isNumberType(type))
        return DataType::eInteger;

    if (isBoolType(type))
        return DataType::eBoolean;


    return DataType::eNull;
}

DbError OraStatement::getColumnInfo(int index, ColumnInfo& info) const noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    info.name = column.name;
    info.type = getColumnType(column.type);
    return DbError::ok();
}

DbError OraStatement::getColumnInfo(std::string_view name, ColumnInfo& info) const noexcept {
    int index = -1;
    if (DbError error = getColumnIndex(name, index))
        return error;

    return getColumnInfo(index, info);
}

DbError OraStatement::getIntByIndex(int index, int64& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    if (column.indicator) {
        value = 0;
        return DbError::ok();
    }

    const CellValue& cell = column.value;

    if (column.type == SQLT_VNU || column.type == SQLT_NUM) {
        long integer;
        if (sword status = OCINumberToInt(mError, &cell.num, sizeof(integer), OCI_NUMBER_SIGNED, &integer)) {
            return oraGetError(mError, status);
        }

        value = integer;
    } else {
        value = cell.integer;
    }

    return DbError::ok();
}

DbError OraStatement::getBooleanByIndex(int index, bool& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    if (column.indicator) {
        value = false;
        return DbError::ok();
    }

    value = getCellBoolean(column);
    return DbError::ok();
}

DbError OraStatement::getStringByIndex(int index, std::string_view& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    if (column.indicator) {
        value = {};
        return DbError::ok();
    }

    value = {(const char*)column.value.text, column.valueLength};
    return DbError::ok();
}

DbError OraStatement::getDoubleByIndex(int index, double& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    if (column.indicator) {
        value = 0.0;
        return DbError::ok();
    }

    value = column.value.real;
    return DbError::ok();
}

DbError OraStatement::getBlobByIndex(int index, Blob& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    if (column.indicator) {
        value = {};
        return DbError::ok();
    }

    const CellValue& cell = column.value;
    const std::uint8_t *bytes = (const std::uint8_t*)cell.lob;

    value = Blob{bytes, bytes + column.valueLength};
    return DbError::ok();
}
