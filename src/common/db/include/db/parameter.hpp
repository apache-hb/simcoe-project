#pragma once

#include "core/adt/combine.hpp"

#include "dao/info.hpp"

#include "db/statement.hpp"

namespace sm::db {
    struct Value {
        std::variant<bool, int64_t, uint64_t, double, std::string, Blob, DateTime, Duration> value;
    };

    class IParam {
    public:
        virtual ~IParam() = default;

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

    class PositionalParam final : public IParam {
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

    class NamedParam final : public IParam {
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

    class Param final : public IParam {
        sm::adt::Combine<IParam, PositionalParam, NamedParam> mImpl;

    public:
        Param(std::string_view name);
        Param(int index);

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

    template<typename T>
    Param param(std::string_view name);

    template<typename T>
    Param param(int index);

    Value value(auto it) {
        return Value{std::move(it)};
    }
}
