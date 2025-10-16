// Copyright (c) 2025 Half_nothing MIT License

#pragma once

#include "windows.h"
#include <cstdint>
#include <minwindef.h>

namespace FSUIPC {
    enum COMSource {
        COM1Active = 0,
        COM1Standby,
        COM2Active,
        COM2Standby
    };

    enum SimConnectionStatus {
        NO_CONNECTION = 0,
        CONNECTED
    };

    enum ApiVersion {
        API_UNKNOWN = 0,
        API_VER1,
        API_VER2
    };

    enum class Simulator {
        ANY = 0,
        FS98 = 1,
        FS2K = 2,
        CFS2 = 3,
        CFS1 = 4,
        FLY = 5,
        FS2K2 = 6,
        FS2K4 = 7,
        FSX = 8,
        ESP = 9,
        P3D = 10
    };

    enum class Error {
        OK = 0,
        ALREADY_OPEN = 1,
        NO_SIMULATOR = 2,
        REGISTER_MESSAGE = 3,
        CREATE_ATOM = 4,
        CREATE_MAPPING = 5,
        CREATE_VIEW = 6,
        VERSION_MISMATCH = 7,
        WRONG_SIMULATOR = 8,
        NOT_OPEN = 9,
        NO_DATA_FOUND = 10,
        TIMEOUT = 11,
        SEND_MESSAGE = 12,
        BAD_DATA = 13,
        NOT_RUNNING = 14,
        BUFFER_FULL = 15
    };

    struct VersionInfo {
        uint32_t fsuipc;
        uint32_t simulator;
        uint32_t library;
    };

    enum class MessageType {
        READ = 1, WRITE = 2
    };

    struct ReadHeader {
        DWORD id;
        DWORD offset;
        DWORD size;
        DWORD targetId;
    };

    struct WriteHeader {
        DWORD id;
        DWORD offset;
        DWORD size;
    };

    struct State {
        HWND hWnd = nullptr;
        UINT msg = 0;
        ATOM atom = 0;
        HANDLE hMap = nullptr;
        BYTE *pView = nullptr;
        BYTE *pNext = nullptr;
        VersionInfo version{};

        ~State() noexcept {
            reset();
        }

        void reset() noexcept;
    };

    struct ReadDataWORD {
        uint32_t offset;
        size_t size;
        WORD data;

        explicit constexpr ReadDataWORD(uint32_t off) : offset(off), size(sizeof(WORD)), data(0) {}
    };

    struct COM1ActiveVer1 : ReadDataWORD {
        COM1ActiveVer1() : ReadDataWORD(0x034E) {}
    };

    struct COM2ActiveVer1 : ReadDataWORD {
        COM2ActiveVer1() : ReadDataWORD(0x3118) {}
    };

    struct COM1StandbyVer1 : ReadDataWORD {
        COM1StandbyVer1() : ReadDataWORD(0x311A) {}
    };

    struct COM2StandbyVer1 : ReadDataWORD {
        COM2StandbyVer1() : ReadDataWORD(0x311C) {}
    };

    struct ReadDataBYTE {
        uint32_t offset;
        size_t size;
        BYTE data;

        explicit constexpr ReadDataBYTE(uint32_t off) : offset(off), size(sizeof(BYTE)), data(0) {}
    };

    struct RadioSwitch: ReadDataBYTE {
        RadioSwitch() : ReadDataBYTE(0x3122) {}
    };

    struct ReadDataDWORD {
        uint32_t offset;
        size_t size;
        DWORD data;

        explicit constexpr ReadDataDWORD(uint32_t off) : offset(off), size(sizeof(DWORD)), data(0) {}
    };

    struct COM1ActiveVer2 : ReadDataDWORD {
        COM1ActiveVer2() : ReadDataDWORD(0x05C4) {}
    };

    struct COM2ActiveVer2 : ReadDataDWORD {
        COM2ActiveVer2() : ReadDataDWORD(0x05C8) {}
    };

    struct COM1StandbyVer2 : ReadDataDWORD {
        COM1StandbyVer2() : ReadDataDWORD(0x05CC) {}
    };

    struct COM2StandbyVer2 : ReadDataDWORD {
        COM2StandbyVer2() : ReadDataDWORD(0x05D0) {}
    };

    class FSUIPCClient;
}
