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

rm::PCIConfigParam rm::DriverHandle::readPciConfig(uint32_t action, uint32_t size, uint32_t word1, uint32_t word2, uint32_t dataSize) {
    rm::PCIConfigParam param = {
        .action = action,
        .size = size,
        .word1 = word1,
        .word2 = word2,
        .dataSize = dataSize,
    };

    std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(dataSize + sizeof(rm::PCIConfigParam));
    memcpy(buffer.get(), &param, sizeof(rm::PCIConfigParam));
    memset(buffer.get() + sizeof(rm::PCIConfigParam), 0, dataSize);

    DWORD returned;
    BOOL bSuccess = DeviceIoControl(handle, kIoControl0, &param, sizeof(rm::PCIConfigParam), buffer.get(), 20 + dataSize, &returned, nullptr);
    if (!bSuccess) {
        throw std::runtime_error("Failed to read PCI config. " + lastErrorMessage());
    }

    std::cout << std::format("readPciConfig: {} (action={}, size={}, word1={}, word2={}, dataSize={})\n",
        returned, param.action,
        param.size, param.word1,
        param.word2, param.dataSize
    );

    for (uint32_t i = 0; i < dataSize; i++) {
        std::cout << std::format("  (buffer[{}]={})\n", i, (uint8_t)buffer[i + sizeof(rm::PCIConfigParam)]);
    }

    return param;
}

void rm::DriverHandle::readMomentaryMemory(uint32_t size) {
    uint32_t header = size;
    std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(size + sizeof(uint32_t));
    memcpy(buffer.get(), &header, sizeof(uint32_t));
    memset(buffer.get() + sizeof(uint32_t), 0, size);

    DWORD returned;
    BOOL bSuccess = DeviceIoControl(handle, kReadMomentaryMemory, buffer.get(), sizeof(uint32_t), buffer.get(), size + sizeof(uint32_t), &returned, nullptr);
    if (!bSuccess) {
        throw std::runtime_error("Failed to read momentary memory. " + lastErrorMessage());
    }

    std::cout << std::format("readMomentaryMemory: {}\n", size);

    for (uint32_t i = 0; i < size; i++) {
        std::cout << std::format("  (buffer[{}]={})\n", i, (uint8_t)buffer[i + sizeof(rm::PCIConfigParam)]);
    }
}

struct Param {
    uint32_t header;
    uint32_t first;
    uint32_t second;
};

uint32_t rm::DriverHandle::readMsrData(uint32_t index) {
    Param param{};
    param.header = index;

    DWORD returned;
    BOOL bSuccess = DeviceIoControl(handle, kIoReadMsr, &param, sizeof(param), &param, sizeof(param), &returned, nullptr);
    if (!bSuccess) {
        throw std::runtime_error("Failed to read msr data. " + lastErrorMessage());
    }

    std::cout << std::format("readMsrData: (header={}, first={}, second={})\n", param.header, param.first, param.second);

    return param.header;
}

struct PciConfigData {
    uint32_t bus;
    uint32_t device;
    uint32_t function;
    char buffer[40];
};

struct MmioConfigParam {
    uint32_t count0;
    uint32_t count1;
    uint32_t count2;
    uint32_t count3;

    PciConfigData data[256];
};

// needs to be at least 28 bytes large
struct MmioSizeRequest {
    uint32_t count;
    char padding[24];
};

void rm::DriverHandle::readMmioConfig() {
    MmioSizeRequest request{};
    DWORD returned;
    BOOL bSuccess = DeviceIoControl(handle, kIoControl5, &request, sizeof(MmioSizeRequest), &request, sizeof(MmioSizeRequest), &returned, nullptr);
    if (!bSuccess) {
        throw std::runtime_error("Failed to read mmio config size. " + lastErrorMessage());
    }

    std::cout << "readMmioConfig: " << request.count << '\n';
#if 0
    std::unique_ptr<MmioConfigParam> param = std::make_unique<MmioConfigParam>();
    param->count0 = count;
    param->count1 = count;
    param->count2 = count;
    param->count3 = count;

    DWORD dwBytesReturned;
    BOOL bSuccess = DeviceIoControl(handle, kIoControl5, param.get(), sizeof(MmioConfigParam), param.get(), sizeof(MmioConfigParam), &dwBytesReturned, nullptr);
    if (!bSuccess) {
        throw std::runtime_error("Failed to read mmio config. " + lastErrorMessage());
    }

    std::cout << "readMmioConfig: " << param->count0 << ", " << param->count1 << ", " << param->count2 << ", " << param->count3 << '\n';
    for (uint32_t i = 0; i < count; i++) {
        std::cout << std::format("  (bus={}, device={}, function={}, buffer={})\n", param->data[i].bus, param->data[i].device, param->data[i].function, param->data[i].buffer);
    }
#endif
}

void rm::DriverHandle::readMemory() {
    struct SetupInfo {
        uint32_t size;
        std::byte buffer[0x5d0];
    };
    SetupInfo value { .size = 0x5d0 };
    DWORD returned;
    BOOL bSuccess = DeviceIoControl(handle, kReadMemory, &value, 4, &value, sizeof(value), &returned, nullptr);
    if (!bSuccess) {
        throw std::runtime_error("Failed to do setup. " + lastErrorMessage());
    }

    std::cout << "readMemory: " << value.size << '\n';
    std::cout << "readMemory: " << value.buffer << '\n';
}