#pragma once

#include "core/macros.hpp"

#include <atomic>

namespace sm {
    template<typename T, typename TDelete, T TEmpty = T{}>
    class SharedHandle {
        struct ControlBlock {
            T mHandle = TEmpty;
            std::atomic<int_fast32_t> mRefCount = 0;

            int_fast32_t addRef() noexcept {
                return mRefCount.fetch_add(1, std::memory_order_relaxed);
            }

            bool dropRef() noexcept {
                return mRefCount.fetch_sub(1, std::memory_order_relaxed) == 1;
            }
        };

        ControlBlock *mControlBlock = nullptr;

        SM_NO_UNIQUE_ADDRESS TDelete mDelete{};

        constexpr void dropRef() noexcept {
            if (mControlBlock == nullptr)
                return;

            if (mControlBlock->dropRef()) {
                mDelete(mControlBlock->mHandle);
                delete mControlBlock;
            }
        }

        constexpr void addRef() noexcept {
            if (mControlBlock == nullptr)
                return;

            mControlBlock->addRef();
        }

        constexpr void replace(ControlBlock *controlBlock) noexcept {
            dropRef();

            mControlBlock = controlBlock;
        }

    public:
        constexpr SharedHandle(T handle = TEmpty, TDelete del = TDelete{}) noexcept
            : mControlBlock(new ControlBlock{handle, 1})
            , mDelete(del)
        { }

        constexpr SharedHandle(const SharedHandle &other) noexcept
            : mControlBlock(other.mControlBlock)
            , mDelete(other.mDelete)
        {
            addRef();
        }

        constexpr SharedHandle &operator=(const SharedHandle &other) noexcept {
            if (this != &other) {
                replace(other.mControlBlock);
                mDelete = other.mDelete;

                addRef();
            }

            return *this;
        }

        constexpr SharedHandle(SharedHandle &&other) noexcept
            : mControlBlock(other.mControlBlock)
            , mDelete(other.mDelete)
        {
            other.mControlBlock = nullptr;
        }

        constexpr SharedHandle &operator=(SharedHandle &&other) noexcept {
            if (this != &other) {
                replace(other.mControlBlock);
                mDelete = other.mDelete;
                other.mControlBlock = nullptr;
            }

            return *this;
        }

        constexpr ~SharedHandle() noexcept {
            dropRef();
        }
    };

    template<typename T, typename TDelete>
    class SharedPtr : public SharedHandle<T*, TDelete, nullptr> {
        using Super = SharedHandle<T*, TDelete, nullptr>;
    public:
        using Super::Super;
    };
}
