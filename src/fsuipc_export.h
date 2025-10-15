// Copyright (c) 2025 Half_nothing MIT License

#pragma once

#include "fsuipc_definition.h"

#define DLL_EXPORT extern "C" __declspec(dllexport)

typedef struct ReturnValue {
    bool requestStatus{false};
    const char *errMessage{"No error found"};
    uint32_t frequency[4]{};
    uint32_t status{FSUIPC::SimConnectionStatus::NO_CONNECTION};
} ReturnValue;

DLL_EXPORT ReturnValue *OpenFSUIPCClient();
DLL_EXPORT ReturnValue *ReadFrequencyInfo();
DLL_EXPORT ReturnValue *CloseFSUIPCClient();
DLL_EXPORT ReturnValue *GetConnectionState();
DLL_EXPORT void FreeMemory(ReturnValue *);
