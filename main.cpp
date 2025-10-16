// Copyright (c) 2025 Half_nothing MIT License

#include "fsuipc_client.h"
#include "fsuipc_export.h"
#include <string>
#include <sstream>

FSUIPC::FSUIPCClient client;
FSUIPC::SimConnectionStatus status = FSUIPC::SimConnectionStatus::NO_CONNECTION;
FSUIPC::ApiVersion apiVersion = FSUIPC::ApiVersion::API_UNKNOWN;

FSUIPC::COM1ActiveVer1 com1ActiveVer1;
FSUIPC::COM1StandbyVer1 com1StandbyVer1;
FSUIPC::COM2ActiveVer1 com2ActiveVer1;
FSUIPC::COM2StandbyVer1 com2StandbyVer1;

FSUIPC::COM1ActiveVer2 com1ActiveVer2;
FSUIPC::COM1StandbyVer2 com1StandbyVer2;
FSUIPC::COM2ActiveVer2 com2ActiveVer2;
FSUIPC::COM2StandbyVer2 com2StandbyVer2;

FSUIPC::RadioSwitch radioSwitch;

uint32_t com1ActiveLast = 0;
uint32_t com1StandbyLast = 0;
uint32_t com2ActiveLast = 0;
uint32_t com2StandbyLast = 0;
uint32_t com1Active = 0;
uint32_t com1Standby = 0;
uint32_t com2Active = 0;
uint32_t com2Standby = 0;

uint32_t processNumber(int);

void updateSimConnection(FSUIPC::SimConnectionStatus);

void disconnect();

void readFrequencyVer1();

void readFrequencyVer2();

void processFrequencyData();

DLL_EXPORT [[maybe_unused]] ReturnValue *OpenFSUIPCClient() {
    auto *returnValue = new ReturnValue();
    if (client.open()) {
        returnValue->requestStatus = true;
        updateSimConnection(FSUIPC::CONNECTED);
        apiVersion = client.getApiVersion();
    } else {
        returnValue->errMessage = client.getLastErrorMessage();
    }
    return returnValue;
}

DLL_EXPORT [[maybe_unused]] ReturnValue *ReadFrequencyInfo() {
    auto *returnValue = new ReturnValue();
    if (status != FSUIPC::CONNECTED) {
        returnValue->requestStatus = false;
        returnValue->errMessage = "FSUIPC not connected";
        return returnValue;
    }
    if (apiVersion == FSUIPC::ApiVersion::API_UNKNOWN) {
        returnValue->requestStatus = false;
        returnValue->errMessage = "Unsupported FSUIPC api version";
        return returnValue;
    }
    client.readBYTE(radioSwitch);
    if (apiVersion == FSUIPC::ApiVersion::API_VER1) {
        readFrequencyVer1();
    } else {
        readFrequencyVer2();
    }
    processFrequencyData();

    returnValue->frequency[0] = com1Active;
    returnValue->frequency[1] = com1Standby;
    returnValue->frequency[2] = com2Active;
    returnValue->frequency[3] = com2Standby;

    returnValue->frequencyFlag = radioSwitch.data;

    if (client.getLastError() == FSUIPC::Error::OK) {
        returnValue->requestStatus = true;
        return returnValue;
    }
    returnValue->errMessage = client.getLastErrorMessage();
    return returnValue;
}

DLL_EXPORT [[maybe_unused]] ReturnValue *CloseFSUIPCClient() {
    auto *returnValue = new ReturnValue();
    disconnect();
    if (client.getLastError() == FSUIPC::Error::OK) {
        returnValue->requestStatus = true;
        return returnValue;
    }
    returnValue->errMessage = client.getLastErrorMessage();
    return returnValue;
}

DLL_EXPORT [[maybe_unused]] ReturnValue *GetConnectionState() {
    auto *returnValue = new ReturnValue();
    returnValue->requestStatus = true;
    returnValue->status = status;
    return returnValue;
}

DLL_EXPORT [[maybe_unused]] void FreeMemory(ReturnValue *pointer) {
    delete pointer;
}

uint32_t processNumber(int n) {
    std::stringstream ss;
    ss << std::hex << n;
    std::string hexStr = ss.str();

    uint32_t converted = stol(hexStr);

    return (uint32_t) (converted * 10 + 100000 + (converted % 5) * 2.5) * 1000;
}

void updateSimConnection(FSUIPC::SimConnectionStatus connectionStatus) {
    status = connectionStatus;
}

void disconnect() {
    if (status == FSUIPC::CONNECTED) {
        client.close();
        apiVersion = FSUIPC::ApiVersion::API_UNKNOWN;
        updateSimConnection(FSUIPC::NO_CONNECTION);
    }
}

void readFrequencyVer1() {
    client.readWORD(com1ActiveVer1);
    client.readWORD(com1StandbyVer1);
    client.readWORD(com2ActiveVer1);
    client.readWORD(com2StandbyVer1);
    client.process();
    com1Active = processNumber(com1ActiveVer1.data);
    com1Standby = processNumber(com1StandbyVer1.data);
    com2Active = processNumber(com2ActiveVer1.data);
    com2Standby = processNumber(com2StandbyVer1.data);
}

void readFrequencyVer2() {
    client.readDWORD(com1ActiveVer2);
    client.readDWORD(com1StandbyVer2);
    client.readDWORD(com2ActiveVer2);
    client.readDWORD(com2StandbyVer2);
    client.process();
    com1Active = com1ActiveVer2.data;
    com1Standby = com1StandbyVer2.data;
    com2Active = com2ActiveVer2.data;
    com2Standby = com2StandbyVer2.data;
}

void processFrequencyData() {
    if (com1Active == 0 && com1Standby == 0 && com2Active == 0 && com2Standby == 0) {
        com1ActiveLast = com1StandbyLast = com2ActiveLast = com2StandbyLast = 0;
        disconnect();
        return;
    }
    if (com1Active != com1ActiveLast) { com1ActiveLast = com1Active; }
    if (com1Standby != com1StandbyLast) { com1StandbyLast = com1Standby; }
    if (com2Active != com2ActiveLast) { com2ActiveLast = com2Active; }
    if (com2Standby != com2StandbyLast) { com2StandbyLast = com2Standby; }
}
