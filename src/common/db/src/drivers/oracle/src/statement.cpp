#include "stdafx.hpp"

#include "oracle/statement.hpp"
#include "oracle/environment.hpp"
#include "oracle/connection.hpp"

using namespace sm;
using namespace sm::db;
using namespace sm::db::oracle;

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

static bool isFloatType(ub2 type) noexcept {
    return type == SQLT_BDOUBLE;
}

static bool isNumberType(ub2 type) noexcept {
    return type == SQLT_NUM || type == SQLT_INT || type == SQLT_VNU;
}

static bool isBoolType(ub2 type) noexcept {
    return type == SQLT_BOL;
}

static bool isDateType(ub2 type) noexcept {
    return type == SQLT_TIMESTAMP;
}

static bool getCellBoolean(const OraColumnInfo& column) noexcept {
    if (isBoolType(column.type))
        return column.value.bol;

    return column.value.text[0] != '0';
}

static DbResult<CellInfo> initCellValue(OraEnvironment& env, OraError error, OraColumnInfo& column) noexcept {
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

    if (isDateType(column.type)) {
        if (sword result = OCIDescriptorAlloc(env.env(), (void**)&column.value.date, OCI_DTYPE_TIMESTAMP, 0, nullptr))
            return std::unexpected(oraGetError(error, result));

        return CellInfo { (void*)&column.value.date, sizeof(column.value.date) };
    }

    switch (column.type) {
    case SQLT_VNU:
        OCINumberSetZero(error, &column.value.num);
        return CellInfo { &column.value.num, sizeof(column.value.num) };

    case SQLT_INT:
        return CellInfo { &column.value.integer, sizeof(column.value.integer) };

    case SQLT_BOL:
        return CellInfo { &column.value.bol, sizeof(column.value.bol) };

    case SQLT_BDOUBLE:
        return CellInfo { &column.value.bdouble, sizeof(column.value.bdouble) };

    default:
        return std::unexpected{DbError::todo(fmt::format("Unsupported data type {}", column.type))};
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

static ub2 remapDataType(ub2 type, bool hasBoolType) noexcept {
    switch (type) {
    case SQLT_NUM: return SQLT_VNU;
    case SQLT_CHR: return SQLT_STR;
    case SQLT_BLOB: return SQLT_BIN;
    case SQLT_DAT: return SQLT_TIMESTAMP;
    case SQLT_BOL: return hasBoolType ? SQLT_BOL : SQLT_CHR;

    case SQLT_IBDOUBLE: case SQLT_IBFLOAT:
        return SQLT_BDOUBLE;

    default:
        return type;
    }
}

static DbResult<std::vector<OraColumnInfo>> defineColumns(OraEnvironment& env, OraError error, OraStmt stmt, bool hasBoolType) noexcept {
    std::vector<OraColumnInfo> columns;
    ub4 params = TRY_RESULT(stmt.getAttribute<ub4>(error, OCI_ATTR_PARAM_COUNT));
    columns.reserve(params + 1);

    for (ub4 i = 1; i <= params; i++) {
        OraParam param;
        sword result = OCIParamGet(stmt, OCI_HTYPE_STMT, error, param.voidpp(), i);
        if (result != OCI_SUCCESS)
            return std::unexpected(oraGetError(error, result));

        ub2 dataType = TRY_RESULT(getColumnType(error, param));
        dataType = remapDataType(dataType, hasBoolType);

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

        auto [value, size] = TRY_RESULT(initCellValue(env, error, info));

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

DbError OraStatement::closeColumns() noexcept {
    for (OraColumnInfo& column : mColumnInfo) {
        if (isStringType(column.type)) {
            mEnvironment.free(column.value.text);
        } else if (isBlobType(column.type)) {
            mEnvironment.free(column.value.lob);
        } else if (isDateType(column.type)) {
            if (DbError error = column.value.date.close(mError))
                return error;
        }
    }

    mColumnInfo.clear();
    return DbError::ok();
}

template<typename... T>
struct overloaded : T... {
    using T::operator()...;
};

void OraStatement::freeBindValues() noexcept {
    while (!mBindValues.empty()) {
        auto& value = mBindValues.front();
        std::visit(overloaded {
            [&](OraDateTime& date) {
                if (DbError error = date.close(mError))
                    LOG_WARN(DbLog, "Failed to close date: {}", error.message());
            },
            [](auto&) { }
        }, value);

        mBindValues.pop_front();
    }
}

DbError OraStatement::executeStatement(ub4 flags, int iters) noexcept {
    if (DbError error = closeColumns())
        return error;

    sword status = OCIStmtExecute(mConnection.service(), mStatement, mError, iters, 0, nullptr, nullptr, flags);

    if (status == OCI_NO_DATA)
        status = OCI_SUCCESS;

    if (!isSuccess(status))
        return oraGetError(mError, status);

    mColumnInfo = TRY_UNWRAP(defineColumns(mEnvironment, mError, mStatement, mConnection.hasBoolType()));

    return DbError::ok();
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

static ub4 computeFlags(bool isQuery, bool autoCommit) noexcept {
    if (isQuery)
        return OCI_STMT_SCROLLABLE_READONLY;

    return autoCommit ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT;
}

DbError OraStatement::start(bool autoCommit, StatementType type) noexcept {
    bool isQuery = (type == StatementType::eQuery);
    int iters = isQuery ? 0 : 1;
    ub4 flags = computeFlags(isQuery, autoCommit);
    if (DbError error = executeStatement(flags, iters))
        return error;

    return isQuery ? next() : DbError::ok();
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
    int64_t *data = addBindValue(value);
    return bindAtPosition(index, data, sizeof(*data), SQLT_INT);
}

struct CellBindInfo {
    const void *ptr;
    ub4 size;
    ub2 type;
};

// TODO: https://docs.oracle.com/en/database/oracle/oracle-database/19/lnoci/binding-and-defining-in-oci.html#GUID-415F2F47-03BA-4E3D-B622-A799409DA243

static CellBindInfo getBoolBindingValue(bool value, bool hasBoolType) noexcept {
    if (hasBoolType) {
        static constexpr bool kValues[] = {false, true};
        return { (void*)&kValues[value], sizeof(value), SQLT_BOL };
    } else {
        static constexpr const char *kValues[] = {"0", "1"};
        return { (void*)kValues[value], 1, SQLT_CHR };
    }
}

DbError OraStatement::bindBooleanByIndex(int index, bool value) noexcept {
    auto [ptr, size, type] = getBoolBindingValue(value, mConnection.hasBoolType());
    return bindAtPosition(index, (void*)ptr, size, type);
}

DbError OraStatement::bindStringByIndex(int index, std::string_view value) noexcept {
    std::string *data = addBindValue(std::string(value));
    return bindAtPosition(index, data->data(), data->size(), SQLT_CHR);
}

DbError OraStatement::bindDoubleByIndex(int index, double value) noexcept {
    double *data = addBindValue(value);
    return bindAtPosition(index, data, sizeof(double), SQLT_FLT);
}

DbError OraStatement::bindBlobByIndex(int index, Blob value) noexcept {
    Blob *data = addBindValue(std::move(value));
    return bindAtPosition(index, data->data(), data->size(), SQLT_BIN);
}

DbError OraStatement::initDateTime(DateTime initial, OraDateTime &result) noexcept {
    OCIDateTime *handle = nullptr;
    if (sword status = OCIDescriptorAlloc(mEnvironment.env(), (void**)&handle, OCI_DTYPE_TIMESTAMP, 0, nullptr))
        return oraGetError(mError, status);

    OraDateTime datetime{handle};

    auto [year, month, day, hour, minute, second, fsec] = DateTimeComponents::fromDateTime(initial);

    sword status = OCIDateTimeConstruct(mEnvironment.env(), mError, datetime, year, month, day, hour, minute, second, fsec, nullptr, 0);
    if (status != OCI_SUCCESS) {
        return oraGetError(mError, status);
    }

    result = datetime;
    return DbError::ok();
}

DbError OraStatement::bindDateTimeByIndex(int index, DateTime value) noexcept {
    OraDateTime datetime;
    if (DbError error = initDateTime(value, datetime))
        return error;

    OCIDateTime **addr = addBindValue(datetime);

    return bindAtPosition(index, (void*)addr, sizeof(*addr), SQLT_TIMESTAMP);
}

DbError OraStatement::bindNullByIndex(int index) noexcept {
    return bindAtPosition(index, nullptr, 0, SQLT_STR);
}

DbError OraStatement::bindIntByName(std::string_view name, int64 value) noexcept {
    int64_t *addr = addBindValue(value);
    return bindAtName(name, addr, sizeof(*addr), SQLT_INT);
}

DbError OraStatement::bindBooleanByName(std::string_view name, bool value) noexcept {
    auto [ptr, size, type] = getBoolBindingValue(value, mConnection.hasBoolType());
    return bindAtName(name, (void*)ptr, size, type);
}

DbError OraStatement::bindStringByName(std::string_view name, std::string_view value) noexcept {
    std::string *addr = addBindValue(std::string(value));
    return bindAtName(name, addr->data(), addr->size(), SQLT_CHR);
}

DbError OraStatement::bindDoubleByName(std::string_view name, double value) noexcept {
    double *addr = addBindValue(value);
    return bindAtName(name, addr, sizeof(*addr), SQLT_FLT);
}

DbError OraStatement::bindBlobByName(std::string_view name, Blob value) noexcept {
    Blob *addr = addBindValue(std::move(value));
    return bindAtName(name, addr->data(), addr->size(), SQLT_BIN);
}

DbError OraStatement::bindDateTimeByName(std::string_view name, DateTime value) noexcept {
    OraDateTime datetime;
    if (DbError error = initDateTime(value, datetime))
        return error;

    OCIDateTime **addr = addBindValue(datetime);
    return bindAtName(name, (void*)addr, sizeof(*addr), SQLT_TIMESTAMP);
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

    if (isFloatType(type))
        return DataType::eDouble;

    if (isNumberType(type))
        return DataType::eInteger;

    if (isBoolType(type))
        return DataType::eBoolean;

    if (isDateType(type))
        return DataType::eDateTime;

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

    value = column.value.bdouble;
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

DbError OraStatement::extractDateTime(const OraDateTime& datetime, DateTime& result) noexcept {
    sb2 year;
    ub1 month, day;
    if (sword status = OCIDateTimeGetDate(mEnvironment.env(), mError, datetime, &year, &month, &day))
        return oraGetError(mError, status);

    ub1 hour, minute, second;
    ub4 fsec;
    if (sword status = OCIDateTimeGetTime(mEnvironment.env(), mError, datetime, &hour, &minute, &second, &fsec))
        return oraGetError(mError, status);

    DateTimeComponents components { year, month, day, hour, minute, second, fsec };
    result = components.toDateTime();

    return DbError::ok();
}

DbError OraStatement::getDateTimeByIndex(int index, DateTime& value) noexcept {
    const OraColumnInfo& column = mColumnInfo[index];
    if (column.indicator) {
        value = {};
        return DbError::ok();
    }

    return extractDateTime(column.value.date, value);
}
