#pragma once

namespace sm::adt {
    template<typename T>
    struct Value {
        T value;
    };

    template<typename E>
    struct Error {
        E error;
    };

    template<typename T, typename E>
    union ResultStorage {
        ~ResultStorage() noexcept { }

        T value;
        E error;
    };

    template<typename T, typename E>
    class Result {
        ResultStorage<T, E> mStorage;
        bool mSuccess;

        constexpr void uninitializedCopyConstruct(const Result& other) {
            if (other.isValue()) {
                std::construct_at(&mStorage.value, other.mStorage.value);
            } else {
                std::construct_at(&mStorage.error, other.mStorage.error);
            }

            mSuccess = other.isValue();
        }

        constexpr void initializedCopyConstruct(const Result& other) {
            if (isValue() && other.isValue()) {
                mStorage.value = other.mStorage.value;
            } else if (isError() && other.isError()) {
                mStorage.error = other.mStorage.error;
            } else {
                destroyStorage();
                uninitializedCopyConstruct(other);
            }
        }

        constexpr void uninitializedMoveConstruct(Result&& other) {
            if (other.isValue()) {
                std::construct_at(&mStorage.value, std::move(other.mStorage.value));
            } else {
                std::construct_at(&mStorage.error, std::move(other.mStorage.error));
            }

            mSuccess = other.isValue();
        }

        constexpr void initializedMoveConstruct(Result&& other) {
            if (isValue() && other.isValue()) {
                mStorage.value = other.mStorage.value;
            } else if (isError() && other.isError()) {
                mStorage.error = other.mStorage.error;
            } else {
                destroyStorage();
                uninitializedMoveConstruct(other);
            }
        }

        constexpr void destroyStorage() {
            if (isValue()) {
                mStorage.value.~T();
            } else {
                mStorage.value.~E();
            }
        }

    public:
        constexpr Result(Value<T> value)
            : mStorage(value.value)
            , mSuccess(true)
        { }

        constexpr Result(Error<E> error)
            : mStorage(error.error)
            , mSuccess(false)
        { }

        constexpr Result(const Result& other) {
            uninitializedCopyConstruct(other);
        }

        constexpr Result& operator=(const Result& other) {
            if (this != &other) {
                initializedCopyConstruct(other);
            }
        }

        constexpr Result(Result&& other) noexcept {
            uninitializedMoveConstruct(std::forward<Result>(other));
        }

        constexpr Result& operator=(Result&& other) noexcept {
            initializedMoveConstruct(std::forward<Result>(other));
        }

        constexpr ~Result() noexcept {
            destroyStorage();
        }

        constexpr bool isValue() const noexcept { return mSuccess; }
        constexpr bool isError() const noexcept { return !mSuccess; }

        constexpr const T& value() const& { return mStorage.value; }
        constexpr const E& error() const& { return mStorage.error; }
    };
}
