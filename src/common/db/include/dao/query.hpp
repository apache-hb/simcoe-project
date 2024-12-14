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

    template<typename... T>
    class Select {
    public:
        Select(auto&&... args);

        Select from(auto table);
        Select where(auto expr);
    };

    class Query {
        struct ColumnExpr {
            std::string_view name;
        };

        struct ValueExpr {
            ColumnValue value;
        };

        struct UnaryExpr {
            std::unique_ptr<Query> expr;
            UnaryQueryOp unary;
        };

        struct BinaryExpr {
            std::unique_ptr<Query> lhs;
            std::unique_ptr<Query> rhs;
            BinaryQueryOp binary;
        };

        using ExprData = std::variant<ColumnExpr, ValueExpr, UnaryExpr, BinaryExpr>;

        ExprData mQuery;

        Query(ExprData data) noexcept
            : mQuery(std::move(data))
        { }

    public:
        template<typename... T>
        Query(Select<T...> select) noexcept;

        Query(ColumnValue value) noexcept;

        template<std::integral T>
        Query(T value) noexcept;

        Query(bool value) noexcept;

        QueryOp op() const noexcept;

        // QueryOp::eColumn
        const ColumnInfo& column() const noexcept;

        // QueryOp::eValue
        const ColumnValue& value() const noexcept;

        // QueryOp::eUnary
        const Query& expr() const noexcept;
        UnaryQueryOp unary() const noexcept;

        // QueryOp::eBinary
        const Query& lhs() const noexcept;
        const Query& rhs() const noexcept;
        BinaryQueryOp binary() const noexcept;

        Query isNull() const noexcept;
        Query isNotNull() const noexcept;

        friend Query operator||(Query lhs, Query rhs);
        friend Query operator&&(Query lhs, Query rhs);

        friend Query operator==(Query lhs, Query rhs);

        static Query ofColumn(std::string_view name);
        static Query ofValue(ColumnValue value);
        static Query ofUnary(Query expr, UnaryQueryOp unary);
        static Query ofBinary(Query lhs, Query rhs, BinaryQueryOp binary);
    };

    int table(std::string_view name);
}
