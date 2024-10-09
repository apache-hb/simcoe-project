#pragma once

#include <stdint.h>
#include <windows.h>

namespace sm::amd::ryzenmaster {
    // GetPCIConfig
    // size 0x18
    // header 0
    // word1 1
    // word2 0x300
    // word3 4
    // word4 0
    static constexpr uint32_t kIoControl0 = 0x81112f24;

    // read momentary memory
    static constexpr uint32_t kIoControl1 = 0x81113008;
    static constexpr uint32_t kIoControl2 = 0x81113004;
    static constexpr uint32_t kIoControl3 = 0x81113000;
    static constexpr uint32_t kIoControl4 = 0x81112ffc;

    // get mmio region config?
    static constexpr uint32_t kIoControl5 = 0x81112f18;

    // get rsd ptr?
    static constexpr uint32_t kIoControl6 = 0x81112f08;

    // write to an msr, not sure what it does yet
    static constexpr uint32_t kIoControl7 = 0x81112ee4;

    // reads from an msr, not sure what the msr is
    static constexpr uint32_t kIoControl8 = 0x81112ee0;

    // writes to an mmio region
    static constexpr uint32_t kIoControl9 = 0x81112ff8;

    struct PCIConfigParam {
        uint32_t control;
        uint32_t size;

        uint32_t word1;
        uint32_t word2;
        uint32_t word3;
        uint32_t word4;
    };

    struct DriverHandle {
        HANDLE semaphore;
        HANDLE mutex;
        HANDLE handle;

        DriverHandle();

        PCIConfigParam readPciConfig();

        uint32_t readMomentaryMemory();
    };
}
