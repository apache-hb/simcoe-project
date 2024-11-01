#pragma once

#include "dao/info.hpp"

#include "db/statement.hpp"

namespace sm::db {
    class IPlaceholder {
    public:
        virtual void setBool(PreparedStatement& stmt, bool value) = 0;
        virtual void setInt(PreparedStatement& stmt, int64_t value) = 0;
        virtual void setUint(PreparedStatement& stmt, uint64_t value) = 0;
        virtual void setFloat(PreparedStatement& stmt, float value) = 0;
        virtual void setDouble(PreparedStatement& stmt, double value) = 0;
        virtual void setString(PreparedStatement& stmt, std::string_view value) = 0;
        virtual void setBlob(PreparedStatement& stmt, Blob value) = 0;
        virtual void setDateTime(PreparedStatement& stmt, DateTime value) = 0;
        virtual void setDuration(PreparedStatement& stmt, Duration value) = 0;
    };

    class PositionalPlaceholder final : public IPlaceholder {
        int mPosition;

        void setBool(PreparedStatement& stmt, bool value);
        void setInt(PreparedStatement& stmt, int64_t value);
        void setUint(PreparedStatement& stmt, uint64_t value);
        void setFloat(PreparedStatement& stmt, float value);
        void setDouble(PreparedStatement& stmt, double value);
        void setString(PreparedStatement& stmt, std::string_view value);
        void setBlob(PreparedStatement& stmt, Blob value);
        void setDateTime(PreparedStatement& stmt, DateTime value);
        void setDuration(PreparedStatement& stmt, Duration value);
    };

    class NamedPlaceholder final : public IPlaceholder {
        std::string mName;

        void setBool(PreparedStatement& stmt, bool value);
        void setInt(PreparedStatement& stmt, int64_t value);
        void setUint(PreparedStatement& stmt, uint64_t value);
        void setFloat(PreparedStatement& stmt, float value);
        void setDouble(PreparedStatement& stmt, double value);
        void setString(PreparedStatement& stmt, std::string_view value);
        void setBlob(PreparedStatement& stmt, Blob value);
        void setDateTime(PreparedStatement& stmt, DateTime value);
        void setDuration(PreparedStatement& stmt, Duration value);
    };

    class Placeholder final : public IPlaceholder {
        std::variant<PositionalPlaceholder, NamedPlaceholder> mImpl;
        IPlaceholder *mVtable;

    public:
        Placeholder(std::string_view name);
        Placeholder(int index);

        void setBool(PreparedStatement& stmt, bool value);
        void setInt(PreparedStatement& stmt, int64_t value);
        void setUint(PreparedStatement& stmt, uint64_t value);
        void setFloat(PreparedStatement& stmt, float value);
        void setDouble(PreparedStatement& stmt, double value);
        void setString(PreparedStatement& stmt, std::string_view value);
        void setBlob(PreparedStatement& stmt, Blob value);
        void setDateTime(PreparedStatement& stmt, DateTime value);
        void setDuration(PreparedStatement& stmt, Duration value);
    };

    Placeholder param(std::string_view name);
    Placeholder param(int index);

    class StatementParameters {
        std::unordered_multimap<std::string, Placeholder> mPlaceholders;
    };
}
