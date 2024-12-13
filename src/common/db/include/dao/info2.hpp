#pragma once

#include "dao/column.hpp"

#include <string>
#include <string_view>
#include <span>

namespace sm::dao {
    class Column;
    class Table;

    class Column {
        std::string mName;
        std::string mComment;
        ColumnType mType;
        AutoIncrement mAutoIncrement;
        bool mNullable;

    public:
        Column(std::string name, std::string comment, ColumnType type, AutoIncrement autoIncrement, bool nullable)
            : mName(std::move(name))
            , mComment(std::move(comment))
            , mType(type)
            , mAutoIncrement(autoIncrement)
            , mNullable(nullable)
        { }

        std::string_view name() const noexcept { return mName; }
        std::string_view comment() const noexcept { return mComment; }
        ColumnType type() const noexcept { return mType; }
        AutoIncrement autoIncrement() const noexcept { return mAutoIncrement; }
        bool nullable() const noexcept { return mNullable; }
    };

    class Constraint {
        std::string mName;

    protected:
        Constraint(std::string name)
            : mName(std::move(name))
        { }

    public:
        std::string_view name() const noexcept { return mName; }
    };

    class UniqueKey : public Constraint {
        const Column *mColumn;

    public:
        UniqueKey(std::string name, const Column *column)
            : Constraint(std::move(name))
            , mColumn(column)
        { }

        const Column *column() const noexcept { return mColumn; }
    };

    class ForeignKey : public Constraint {
        const Table *mTable;
        const Column *mColumn;
        const Column *mKey;
        OnDelete mOnDelete;
        OnUpdate mOnUpdate;

    public:
        ForeignKey(std::string name, const Table *table, const Column *column, const Column *key, OnDelete onDelete, OnUpdate onUpdate)
            : Constraint(std::move(name))
            , mTable(table)
            , mColumn(column)
            , mKey(key)
            , mOnDelete(onDelete)
            , mOnUpdate(onUpdate)
        { }

        const Table *table() const noexcept { return mTable; }
        const Column *column() const noexcept { return mColumn; }
        const Column *key() const noexcept { return mKey; }
        OnDelete onDelete() const noexcept { return mOnDelete; }
        OnUpdate onUpdate() const noexcept { return mOnUpdate; }
    };

    class Table {
        std::string mSchema;
        std::string mName;
        std::string mComment;

        std::vector<Column> mColumns;

        [[maybe_unused]]
        const Column *mPrimaryKey = nullptr;

        [[maybe_unused]]
        bool mSingleRow = false;;

    public:
        Table(std::string schema, std::string name, std::string comment, std::vector<Column> columns)
            : mSchema(std::move(schema))
            , mName(std::move(name))
            , mComment(std::move(comment))
            , mColumns(std::move(columns))
        { }

        std::string_view schema() const noexcept { return mSchema; }
        std::string_view name() const noexcept { return mName; }
        std::string_view comment() const noexcept { return mComment; }
        std::span<const Column> columns() const noexcept { return mColumns; }
    };
}
