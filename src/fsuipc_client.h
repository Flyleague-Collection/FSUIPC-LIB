// Copyright (c) 2025 Half_nothing MIT License

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "fsuipc_definition.h"

namespace FSUIPC {
    class FSUIPCClient {
    public:
        FSUIPCClient();

        ~FSUIPCClient();

        FSUIPCClient(const FSUIPCClient &) = delete;

        FSUIPCClient &operator=(const FSUIPCClient &) = delete;

        bool open(Simulator requested = Simulator::ANY);

        bool close() noexcept;

        bool isOpen() const noexcept;

        bool getVersion(VersionInfo &version);

        bool read(uint32_t offset, size_t size, void *destination);

        bool readWORD(ReadDataWORD &data);

        bool readDWORD(ReadDataDWORD &data);

        bool write(uint32_t offset, size_t size, const void *source);

        bool process();

        void clearError();

        Error getLastError();

        const char *getLastErrorMessage();

        ApiVersion getApiVersion() const;

    private:
        std::unordered_map<DWORD, void *> dataMap;
        std::unique_ptr<State> state;
        int dataId = 0;
        Error lastError = Error::OK;
        const char *lastErrorMessage{};
        static constexpr size_t MAX_SIZE = 0x7F00;
        ApiVersion apiVersion = API_UNKNOWN;

        void setLastError(Error error, const char *errorMessage);

        bool initializeConnection(Simulator requested);

        bool verifyVersion(Simulator requested);

        bool sendRequests();

        bool processResponses();

        bool checkApiVersion();
    };
}
