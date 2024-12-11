#include "stdafx.hpp"

#include "threads/mutex.hpp"

using namespace sm;
using namespace sm::threads;

Mutex::Mutex() noexcept {
    ::InitializeCriticalSectionEx(&mCriticalSection, 0, 0);
}

Mutex::~Mutex() noexcept {
    ::DeleteCriticalSection(&mCriticalSection);
}

void Mutex::lock() noexcept {
    ::EnterCriticalSection(&mCriticalSection);
}

bool Mutex::try_lock() noexcept {
    return ::TryEnterCriticalSection(&mCriticalSection) != 0;
}

void Mutex::unlock() noexcept {
    ::LeaveCriticalSection(&mCriticalSection);
}
