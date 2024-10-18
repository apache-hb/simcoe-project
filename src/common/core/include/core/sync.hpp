#pragma once

#include <mutex>
#include <shared_mutex>

namespace sm {
    template<typename T, typename M, template<typename> typename G>
    class Guarded {
        G<M> mGuard;
        T &mValue;

    public:
        Guarded(T &value, M &mutex) : mGuard(mutex), mValue(value) { }

        T& operator*() { return mValue; }
        T* operator->() { return &mValue; }
    };

    template<typename T>
    using ExclusiveGuard = Guarded<T, std::mutex, std::lock_guard>;

    template<typename T>
    using ReadGuard = Guarded<const T, std::shared_mutex, std::shared_lock>;

    template<typename T>
    using WriteGuard = Guarded<T, std::shared_mutex, std::unique_lock>;

    template<typename T>
    class ExclusiveSync {
        std::mutex mMutex;
        T mValue;

    public:
        using Guard = ExclusiveGuard<T>;

        ExclusiveSync() = default;
        ExclusiveSync(T initial) : mValue(initial) { }

        void set(T value) {
            std::lock_guard lock(mMutex);
            mValue = value;
        }

        void modify(auto&& fn) {
            std::lock_guard lock(mMutex);
            fn(mValue);
        }

        Guard take() {
            return Guard { mValue, mMutex };
        }
    };

    template<typename T>
    class Sync {
        std::shared_mutex mMutex;
        T mValue;

    public:
        using WriteGuard = WriteGuard<T>;
        using ReadGuard = ReadGuard<T>;

        Sync() = default;
        Sync(T initial) : mValue(initial) { }

        void set(T value) {
            std::lock_guard lock(mMutex);
            mValue = value;
        }

        void modify(auto&& fn) {
            std::lock_guard lock(mMutex);
            fn(mValue);
        }

        void read(auto&& fn) {
            std::shared_lock lock(mMutex);
            fn(const_cast<const T&>(mValue));
        }

        WriteGuard acquire() {
            return WriteGuard { mValue, mMutex };
        }

        ReadGuard read() {
            return ReadGuard { mValue, mMutex };
        }
    };
}
