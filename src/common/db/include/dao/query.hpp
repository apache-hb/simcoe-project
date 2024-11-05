#pragma once

#include "dao/info.hpp"

#include <variant>

namespace sm::dao {
    using ColumnValue = std::variant<
        int32_t,      // ColumnType::eInt
        uint32_t,     // ColumnType::eUint
        int64_t,      // ColumnType::eLong
        uint64_t,     // ColumnType::eUlong
        bool,         // ColumnType::eBool
        std::string,  // ColumnType::eVarChar/ColumnType::eChar
        float,        // ColumnType::eFloat
        double,       // ColumnType::eDouble
        db::Blob,     // ColumnType::eBlob
        db::DateTime  // ColumnType::eDateTime
    >;

    enum class UnaryQueryOp : uint_least8_t {
        eIsNull,
        eIsNotNull,
    };

    enum class BinaryQueryOp : uint_least8_t {
        eEqual,
        eNotEqual,
        eLessThan,
        eLessThanOrEqual,
        eGreaterThan,
        eGreaterThanOrEqual,
        eLike,
    };

    enum class QueryOp : uint_least8_t {
        eColumn,
        eValue,
        eUnary,
        eBinary,
    };

    class QueryExpr {
        struct ColumnExpr {
            const ColumnInfo& column;
        };

        struct ValueExpr {
            ColumnValue value;
        };

        struct UnaryExpr {
            std::unique_ptr<QueryExpr> expr;
            UnaryQueryOp unary;
        };

        struct BinaryExpr {
            std::unique_ptr<QueryExpr> lhs;
            std::unique_ptr<QueryExpr> rhs;
            BinaryQueryOp binary;
        };

        using ExprData = std::variant<ColumnExpr, ValueExpr, UnaryExpr, BinaryExpr>;

        ExprData mQuery;

        template<typename T>
        QueryExpr(const T& data) noexcept
            : mQuery(data)
        { }

    public:
        QueryOp op() const noexcept;

        // QueryOp::eColumn
        const ColumnInfo& column() const noexcept;

        // QueryOp::eValue
        const ColumnValue& value() const noexcept;

        // QueryOp::eUnary
        const QueryExpr& expr() const noexcept;
        UnaryQueryOp unary() const noexcept;

        // QueryOp::eBinary
        const QueryExpr& lhs() const noexcept;
        const QueryExpr& rhs() const noexcept;
        BinaryQueryOp binary() const noexcept;

        friend QueryExpr operator||(QueryExpr lhs, QueryExpr rhs);
        friend QueryExpr operator&&(QueryExpr lhs, QueryExpr rhs);

        static QueryExpr ofColumn(const ColumnInfo& column);
        static QueryExpr ofValue(ColumnValue value);
        static QueryExpr ofUnary(QueryExpr expr, UnaryQueryOp unary);
        static QueryExpr ofBinary(QueryExpr lhs, QueryExpr rhs, BinaryQueryOp binary);
    };

    QueryExpr isNull(QueryExpr expr);
    QueryExpr isNotNull(QueryExpr expr);
}
