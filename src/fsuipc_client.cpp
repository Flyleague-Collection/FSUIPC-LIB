// Copyright (c) 2025 Half_nothing MIT License

#include "fsuipc_client.h"
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <utility>

namespace FSUIPC {
    constexpr char FS6IPC_MSGNAME1[] = "FsasmLib:IPC";
    constexpr DWORD FS6IPC_MESSAGE_SUCCESS = 1;

    void State::reset() noexcept {
        if (atom) {
            GlobalDeleteAtom(atom);
            atom = 0;
        }

        if (pView) {
            UnmapViewOfFile(pView);
            pView = nullptr;
        }

        if (hMap) {
            CloseHandle(hMap);
            hMap = nullptr;
        }

        hWnd = nullptr;
        msg = 0;
        pNext = nullptr;
    }

    FSUIPCClient::FSUIPCClient() : state(std::make_unique<State>()) {
        state->version = {0, 0, 2002};
    }

    FSUIPCClient::~FSUIPCClient() {
        try {
            close();
        } catch (...) {
        }
    }

    bool FSUIPCClient::open(Simulator requested) {
        if (isOpen()) {
            setLastError(Error::ALREADY_OPEN, "The connection has been opened");
            return true;
        }

        state->reset();
        try {
            if (initializeConnection(requested) && verifyVersion(requested)) {
                checkApiVersion();
                clearError();
                return true;
            }
            return false;
        } catch (...) {
            state->reset();
            return false;
        }
    }

    bool FSUIPCClient::close() noexcept {
        if (isOpen()) {
            state->reset();
            clearError();
            return true;
        }
        setLastError(Error::NOT_OPEN, "There is no active connection, can't close connection");
        return false;
    }

    bool FSUIPCClient::isOpen() const noexcept {
        return state && state->pView != nullptr;
    }

    bool FSUIPCClient::getVersion(VersionInfo &version) {
        if (!isOpen()) {
            setLastError(Error::NOT_OPEN, "There is no active connection, can't get FSUIPC version");
            return false;
        }
        version = state->version;
        clearError();
        return true;
    }

    bool FSUIPCClient::read(uint32_t offset, size_t size, void *destination) {
        if (!isOpen()) {
            setLastError(Error::NOT_OPEN, "Connection not open");
            return false;
        }

        size_t requiredSize = sizeof(ReadHeader) + size;
        if ((state->pNext - state->pView) + requiredSize + 4 > MAX_SIZE) {
            setLastError(Error::BUFFER_FULL, "Read request exceeds buffer capacity");
            return false;
        }

        auto *header = reinterpret_cast<ReadHeader *>(state->pNext);
        header->id = static_cast<DWORD>(MessageType::READ);
        header->offset = offset;
        header->size = static_cast<DWORD>(size);
        header->targetId = dataId++;

        dataMap.insert(std::pair(header->targetId, destination));

        BYTE *dataStart = state->pNext + sizeof(ReadHeader);
        ZeroMemory(dataStart, size);

        state->pNext += sizeof(ReadHeader) + size;
        clearError();
        return true;
    }

    bool FSUIPCClient::readWORD(ReadDataWORD &data) {
        return read(data.offset, data.size, &data.data);
    }

    bool FSUIPCClient::readDWORD(ReadDataDWORD &data) {
        return read(data.offset, data.size, &data.data);
    }

    bool FSUIPCClient::write(uint32_t offset, size_t size, const void *source) {
        if (!isOpen()) {
            setLastError(Error::NOT_OPEN, "Connection not open");
            return false;
        }

        size_t requiredSize = sizeof(WriteHeader) + size;
        if ((state->pNext - state->pView) + requiredSize + 4 > MAX_SIZE) {
            setLastError(Error::BUFFER_FULL, "Write request exceeds buffer capacity");
            return false;
        }

        auto *header = reinterpret_cast<WriteHeader *>(state->pNext);
        header->id = static_cast<DWORD>(MessageType::WRITE);
        header->offset = offset;
        header->size = static_cast<DWORD>(size);

        BYTE *dataStart = state->pNext + sizeof(WriteHeader);
        if (source && size > 0) {
            memcpy(dataStart, source, size);
        }

        state->pNext += sizeof(WriteHeader) + size;
        clearError();
        return true;
    }

    bool FSUIPCClient::process() {
        if (!isOpen()) {
            setLastError(Error::NOT_OPEN, "Connection not open");
            return false;
        }

        if (state->pNext == state->pView) {
            setLastError(Error::NO_DATA_FOUND, "No operations to process");
            return false;
        }

        ZeroMemory(state->pNext, 4);
        state->pNext = state->pView;

        sendRequests();
        processResponses();
        clearError();
        return true;
    }

    bool FSUIPCClient::initializeConnection(Simulator requested) {
        static int nTry = 0;
        nTry++;

        state->hWnd = FindWindowEx(nullptr, nullptr, "UIPCMAIN", nullptr);
        if (!state->hWnd) {
            state->hWnd = FindWindowEx(nullptr, nullptr, "FS98MAIN", nullptr);
            if (!state->hWnd) {
                setLastError(Error::NO_SIMULATOR, "Simulator not found");
                return false;
            }
        }

        state->msg = RegisterWindowMessage(FS6IPC_MSGNAME1);
        if (state->msg == 0) {
            setLastError(Error::REGISTER_MESSAGE, "Failed to register window message");
            return false;
        }

        char szName[MAX_PATH];
        wsprintf(szName, std::string(FS6IPC_MSGNAME1).append(":%X:%X").c_str(), GetCurrentProcessId(), nTry);

        state->atom = GlobalAddAtom(szName);
        if (state->atom == 0) {
            setLastError(Error::CREATE_ATOM, "Failed to create global atom");
            return false;
        }

        state->hMap = CreateFileMapping(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0, MAX_SIZE + 256,
                szName);

        if (!state->hMap || GetLastError() == ERROR_ALREADY_EXISTS) {
            setLastError(Error::CREATE_MAPPING, "Failed to create file mapping");
            return false;
        }

        state->pView = static_cast<BYTE *>(MapViewOfFile(
                state->hMap,
                FILE_MAP_WRITE,
                0, 0,
                0));

        if (!state->pView) {
            setLastError(Error::CREATE_VIEW, "Failed to map view of file");
            return false;
        }

        state->pNext = state->pView;
        clearError();
        return true;
    }

    bool FSUIPCClient::verifyVersion(Simulator requested) {
        int attempts = 0;
        const int maxAttempts = 5;

        while (attempts++ < maxAttempts) {
            read(0x3304, 4, &state->version.fsuipc);
            read(0x3308, 4, &state->version.simulator);
            if (attempts == 1) {
                write(0x330a, 2, &state->version.library);
            }
            process();
            if (state->version.fsuipc != 0 &&
                state->version.simulator != 0 &&
                state->version.simulator >= 0x19980005 &&
                (state->version.simulator & 0xFFFF0000) == 0xFADE0000) {
                break;
            }
            Sleep(100);
        }

        if (attempts >= maxAttempts) {
            return false;
        }

        state->version.simulator &= 0xFFFF;

        if (requested != Simulator::ANY &&
            static_cast<uint32_t>(requested) != state->version.simulator) {
            std::ostringstream oss;
            oss << "Wrong simulator version. Expected: "
                << static_cast<int>(requested)
                << ", Found: " << state->version.simulator;
            setLastError(Error::WRONG_SIMULATOR, oss.str().c_str());
            return false;
        }

        clearError();
        return true;
    }

    bool FSUIPCClient::sendRequests() {
        uint64_t dwError = 0;
        int attempts = 0;
        const int maxAttempts = 10;

        while (attempts++ < maxAttempts) {
            if (SendMessageTimeout(
                    state->hWnd,
                    state->msg,
                    state->atom,
                    0,
                    SMTO_BLOCK,
                    2000,
                    &dwError)) {
                break;
            }
            Sleep(100);
        }

        if (attempts >= maxAttempts) {
            DWORD _lastError = GetLastError();
            Error error = (_lastError == 0) ? Error::TIMEOUT : Error::SEND_MESSAGE;

            std::ostringstream oss;
            oss << "Failed to send message after " << maxAttempts << " attempts";
            if (_lastError != 0) {
                oss << ", Win32 error: " << _lastError;
            }

            setLastError(error, oss.str().c_str());
            return false;
        }

        if (dwError != FS6IPC_MESSAGE_SUCCESS) {
            setLastError(Error::BAD_DATA, "FSUIPC rejected the request data");
            return false;
        }

        clearError();
        return true;
    }

    bool FSUIPCClient::processResponses() {
        auto *pdw = reinterpret_cast<DWORD *>(state->pView);

        while (*pdw) {
            switch (*pdw) {
                case static_cast<DWORD>(MessageType::READ): {
                    auto *header = reinterpret_cast<ReadHeader *>(pdw);
                    state->pNext += sizeof(ReadHeader);

                    void *destination = dataMap.at(header->targetId);

                    if (destination && header->size > 0) {
                        memcpy(destination, state->pNext, header->size);
                    }

                    state->pNext += header->size;
                    break;
                }

                case static_cast<DWORD>(MessageType::WRITE): {
                    auto *header = reinterpret_cast<WriteHeader *>(pdw);
                    state->pNext += sizeof(WriteHeader) + header->size;
                    break;
                }

                default:
                    *pdw = 0;
                    break;
            }

            pdw = reinterpret_cast<DWORD *>(state->pNext);
        }

        state->pNext = state->pView;
        return true;
    }

    void FSUIPCClient::clearError() {
        lastError = Error::OK;
        lastErrorMessage = "";
    }

    Error FSUIPCClient::getLastError() {
        return lastError;
    }

    const char * FSUIPCClient::getLastErrorMessage() {
        return lastErrorMessage;
    }

    void FSUIPCClient::setLastError(Error error, const char * errorMessage) {
        lastError = error;
        lastErrorMessage = errorMessage;
    }

    bool FSUIPCClient::checkApiVersion() {
        if (!isOpen()) {
            return false;
        }
        COM1ActiveVer1 ver1;
        COM1ActiveVer2 ver2;
        readWORD(ver1);
        readDWORD(ver2);
        process();
        if (ver2.data != 0) {
            apiVersion = API_VER2;
            return true;
        }
        if (ver1.data != 0) {
            apiVersion = API_VER1;
            return true;
        }
        apiVersion = API_UNKNOWN;
        return false;
    }

    ApiVersion FSUIPCClient::getApiVersion() const {
        return apiVersion;
    }
}
