#include "stdafx.hpp"

#include "threads/mutex.hpp"

using namespace sm;
using namespace sm::threads;

// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-tryentercriticalsection
// msdn indicates that TryEnterCriticalSection is non-blocking

WINBASEAPI
VOID WINAPI EnterCriticalSection(_Inout_ LPCRITICAL_SECTION lpCriticalSection) [[clang::blocking]];

WINBASEAPI
VOID WINAPI LeaveCriticalSection(_Inout_ LPCRITICAL_SECTION lpCriticalSection) [[clang::nonblocking]];

WINBASEAPI
BOOL WINAPI TryEnterCriticalSection(_Inout_ LPCRITICAL_SECTION lpCriticalSection) [[clang::nonblocking]];

Mutex::Mutex() noexcept {
    ::InitializeCriticalSectionEx(&mCriticalSection, 0, 0);
}

Mutex::~Mutex() noexcept {
    ::DeleteCriticalSection(&mCriticalSection);
}

void Mutex::lock() noexcept [[clang::blocking]] {
    ::EnterCriticalSection(&mCriticalSection);
}

bool Mutex::try_lock() noexcept [[clang::nonblocking]] {
    return ::TryEnterCriticalSection(&mCriticalSection) != 0;
}

void Mutex::unlock() noexcept [[clang::nonblocking]] {
    ::LeaveCriticalSection(&mCriticalSection);
}
