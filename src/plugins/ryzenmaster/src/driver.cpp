#include "ryzenmaster/driver.hpp"

#include <iostream>
#include <sddl.h>
#include <comdef.h>

#include <stdexcept>

namespace rm = sm::amd::ryzenmaster;

static constexpr wchar_t kDriverPath[] = L"\\\\.\\AMDRyzenMasterDriverV26";
static constexpr wchar_t kSemaphoreName[] = L"Global\\AMDRyzenMasterSemaphore";
static constexpr wchar_t kMutexName[] = L"Global\\AMDRyzenMasterMutex";
static constexpr wchar_t kServiceName[] = L"AMDRyzenMasterDriverV26";

static constexpr wchar_t kSecurityDecriptor[] = L"D:(D;OICI;GA;;;BG)(D;OICI;GA;;;AN)(A;OICI;GRGWGX;;;AU)(A;OICI;GA;;;BA)\\";

static std::string lastErrorMessage() {
    DWORD dwError = GetLastError();
    _com_error error(dwError);
    return std::format("{} ({})", error.ErrorMessage(), dwError);
}

static PSECURITY_DESCRIPTOR createSecurityDescriptor() {
    PSECURITY_DESCRIPTOR result;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(kSecurityDecriptor, SDDL_REVISION_1, (PSECURITY_DESCRIPTOR*)&result, nullptr)) {
        throw std::runtime_error("Failed to initialize security descriptor. " + lastErrorMessage());
    }
    return result;
}

static HANDLE getGlobalSemaphore() {
    HANDLE semaphore = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, kSemaphoreName);
    if (semaphore == nullptr) {
        PSECURITY_DESCRIPTOR security = createSecurityDescriptor();
        SECURITY_ATTRIBUTES attributes = { sizeof(SECURITY_ATTRIBUTES), security, FALSE };
        semaphore = CreateSemaphoreW(&attributes, 10000, 10000, kSemaphoreName);
        LocalFree(security);
    }

    if (semaphore == nullptr) {
        throw std::runtime_error("Failed to open semaphore handle. " + lastErrorMessage());
    }

    return semaphore;
}

static HANDLE getGlobalMutex() {
    HANDLE mutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, kMutexName);
    if (mutex == nullptr) {
        PSECURITY_DESCRIPTOR security = createSecurityDescriptor();
        SECURITY_ATTRIBUTES attributes = { sizeof(SECURITY_ATTRIBUTES), security, FALSE };
        mutex = CreateMutexW(&attributes, FALSE, kMutexName);
        LocalFree(security);
    }

    if (mutex == nullptr) {
        throw std::runtime_error("Failed to open mutex handle. " + lastErrorMessage());
    }

    return mutex;
}

static SC_HANDLE createDriverService(SC_HANDLE service, const std::wstring& path) {
    return CreateServiceW(
        service, kServiceName, kServiceName,
        SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL, path.c_str(), nullptr,
        nullptr, nullptr, nullptr, nullptr
    );
}

static void startDriverService() {
    WCHAR buffer[1024];
    DWORD used = GetEnvironmentVariableW(L"AMDRMMONITORSDKPATH", buffer, sizeof(buffer));
    if (used == 0) {
        throw std::runtime_error("Failed to open driver handle. " + lastErrorMessage());
    }

    std::wstring path(buffer, used);
    path += L"bin\\AMDRyzenMasterDriver.sys";

    SC_HANDLE service = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (service == nullptr) {
        throw std::runtime_error("Failed to open service manager. " + lastErrorMessage());
    }

    SC_HANDLE driver = createDriverService(service, path);

    if (driver == nullptr) {
        DWORD dwLastError = GetLastError();
        if (dwLastError == ERROR_SERVICE_EXISTS) {
            driver = OpenServiceW(service, kServiceName, SERVICE_ALL_ACCESS);
        } else if (dwLastError == ERROR_SERVICE_MARKED_FOR_DELETE) {
            driver = OpenServiceW(service, kServiceName, SERVICE_ALL_ACCESS);
            SERVICE_STATUS status;
            ControlService(driver, SERVICE_CONTROL_STOP, &status);
            CloseServiceHandle(driver);

            driver = createDriverService(service, path);
        } else {
            CloseServiceHandle(service);
            throw std::runtime_error("Failed to create service driver. " + lastErrorMessage());
        }
    }

    if (driver == nullptr) {
        CloseServiceHandle(service);
        throw std::runtime_error("Failed to create service. " + lastErrorMessage());
    }

    BOOL bSuccess = StartServiceW(driver, 0, nullptr);
    if (!bSuccess) {
        DWORD dwLastError = GetLastError();
        if (dwLastError == ERROR_PATH_NOT_FOUND) {
            if (DeleteService(driver)) {
                CloseServiceHandle(driver);
                driver = createDriverService(service, path);

                if (driver == nullptr) {
                    BOOL bSuccess = StartServiceW(driver, 0, nullptr);
                    if (!bSuccess) {
                        throw std::runtime_error("Failed to recreate service. " + lastErrorMessage());
                    }
                }
            }
        }

        if (dwLastError == ERROR_SERVICE_ALREADY_RUNNING) {
            CloseServiceHandle(driver);
            return;
        }

        throw std::runtime_error("Failed to start service. " + lastErrorMessage());
    }
}

static HANDLE createDriverHandle() {
    DWORD dwDesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA;
    DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD dwCreate = OPEN_EXISTING;
    DWORD dwAttributes = FILE_ATTRIBUTE_NORMAL;
    return CreateFileW(kDriverPath, dwDesiredAccess, dwShareMode, nullptr, dwCreate, dwAttributes, nullptr);
}

rm::DriverHandle::DriverHandle() {
    semaphore = getGlobalSemaphore();
    mutex = getGlobalMutex();

    handle = createDriverHandle();
    if (handle == INVALID_HANDLE_VALUE) {
        startDriverService();
        handle = createDriverHandle();
    }

    if (handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open driver handle. " + lastErrorMessage());
    }
}

rm::PCIConfigParam rm::DriverHandle::readPciConfig() {
    rm::PCIConfigParam param = {
        .control = 0,
        .size = sizeof(rm::PCIConfigParam),
        .word1 = 0,
        .word2 = 200,
        .word3 = 4,
        .word4 = 0,
    };

    DWORD dwBytesReturned;
    BOOL bSuccess = DeviceIoControl(handle, kIoControl0, &param, sizeof(param), &param, sizeof(param), &dwBytesReturned, nullptr);
    if (!bSuccess) {
        throw std::runtime_error("Failed to read PCI config. " + lastErrorMessage());
    }

    std::cout << std::format("readPciConfig: (control={}, size={}, word1={}, word2={}, word3={}, word4={})\n", param.control, param.size, param.word1, param.word2, param.word3, param.word4);

    return param;
}

uint32_t rm::DriverHandle::readMomentaryMemory() {
    uint32_t input = 0;
    uint64_t result;

    DWORD dwBytesReturned;
    BOOL bSuccess = DeviceIoControl(handle, kIoControl1, &input, sizeof(input), &result, sizeof(result), &dwBytesReturned, nullptr);
    if (!bSuccess) {
        throw std::runtime_error("Failed to read momentary memory. " + lastErrorMessage());
    }

    std::cout << std::format("readMomentaryMemory: {}\n", result);

    return result;
}
