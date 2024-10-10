#pragma once

#include <stdint.h>
#include <windows.h>

#include <cstddef>

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
    static constexpr uint32_t kReadMomentaryMemory = 0x81113008;

    static constexpr uint32_t kIoControl2 = 0x81113004;
    static constexpr uint32_t kIoControl3 = 0x81113000;
    static constexpr uint32_t kIoControl4 = 0x81112ffc;

    // get mmio region config?
    static constexpr uint32_t kIoControl5 = 0x81112f18;

    // get rsd ptr?
    static constexpr uint32_t kReadMemory = 0x81112f08;

    // write to an msr, not sure what it does yet
    static constexpr uint32_t kIoControl7 = 0x81112ee4;

    static constexpr uint32_t kIoReadMsr = 0x81112ee0;

    // writes to an mmio region
    static constexpr uint32_t kIoControl9 = 0x81112ff8;

    struct PCIConfigParam {
        uint32_t action;
        uint32_t size;

        uint32_t word1;
        uint32_t word2;
        uint32_t dataSize;
    };

    enum MsrIndex : uint32_t {
        eMsrPstate0 = 0, // 0xC0010064
        eMsrPstate1 = 1, // 0xC0010065
        eMsrPstate2 = 2, // 0xC0010066
        eMsrPstate3 = 3, // 0xC0010067
        eMsrPstate4 = 4, // 0xC0010068

        eMsrUnknown0 = 6, // 0xC0010290
        eMsrUnknown1 = 9, // 0xC0010292
        eMsrUnknown2 = 11, // 0xC0010293

        eMsrUnknown3 = 12, // 0x8B
        eMsrEnableCPPC = 13, // 0xC00102B1
        eMsrUnknown5 = 14, // 0xC00102B3

        eMsrHwcfg = 5, // 0xC0010015

        eMsrPstateControl = 7, // 0xC0010062
        eMsrPstateCurrentLimit = 8, // 0xC0010061

        eMsrPstateStatus = 10, // 0xC0010063
    };

    struct DriverHandle {
        HANDLE semaphore;
        HANDLE mutex;
        HANDLE handle;

        DriverHandle();

        PCIConfigParam readPciConfig(uint32_t header, uint32_t size, uint32_t word1, uint32_t word2, uint32_t dataSize);

        void readMomentaryMemory(uint32_t size);

        uint32_t readMsrData(uint32_t index);

        void readMmioConfig();

        void doSetup();

        void readMemory();
    };
}
