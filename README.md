# FSUIPC LIB

An FSUIPC library to read the simulator radio frequency  
This library is designed to make python easier to call  
The definition of the data structure can be found in this file[`src/fsuipc_export.h`](src/fsuipc_export.h)

You can use example code below

```python
from time import sleep

from ctypes import POINTER, Structure, c_bool, c_char_p, c_int32, cdll
from dataclasses import dataclass


class CReturnValue(Structure):
    _fields_ = [
        ("requestStatus", c_bool),
        ("errMessage", c_char_p),
        ("frequency", c_int32 * 4),
        ("status", c_int32),
    ]


@dataclass
class ReturnValue:
    requestStatus: bool
    errMessage: str
    frequency: list[int]
    status: int


class FSUIPCClient:
    def __init__(self, fsuipc_lib_path: str) -> None:
        fsuipc_lib = cdll.LoadLibrary(fsuipc_lib_path)
        fsuipc_lib.OpenFSUIPCClient.restype = POINTER(CReturnValue)
        fsuipc_lib.ReadFrequencyInfo.restype = POINTER(CReturnValue)
        fsuipc_lib.CloseFSUIPCClient.restype = POINTER(CReturnValue)
        fsuipc_lib.GetConnectionState.restype = POINTER(CReturnValue)
        fsuipc_lib.FreeMemory.argtypes = [POINTER(CReturnValue)]
        fsuipc_lib.FreeMemory.restype = None
        self._fsuipc_lib = fsuipc_lib

    def open_fsuipc_client(self) -> ReturnValue:
        return self._call_function("OpenFSUIPCClient")

    def close_fsuipc_client(self) -> ReturnValue:
        return self._call_function("CloseFSUIPCClient")

    def get_connection_state(self) -> ReturnValue:
        return self._call_function("GetConnectionState")

    def get_frequency(self) -> ReturnValue:
        return self._call_function("ReadFrequencyInfo")

    def _call_function(self, function_name: str) -> ReturnValue:
        if not hasattr(self._fsuipc_lib, function_name):
            raise AttributeError(f"Function {function_name} not available")
        function = getattr(self._fsuipc_lib, function_name)
        function_return = function()
        try:
            result = ReturnValue(function_return.contents.requestStatus,
                                 function_return.contents.errMessage.decode(),
                                 function_return.contents.frequency[:],
                                 function_return.contents.status)
        finally:
            self._free_memory(function_return)
        return result

    def _free_memory(self, pointer) -> None:
        self._fsuipc_lib.FreeMemory(pointer)


fsuipc_client = FSUIPCClient("./libfsuipc.dll")

while True:
    res = fsuipc_client.open_fsuipc_client()
    if res.requestStatus:
        print(f"FSUIPC connection established")
        break
    print(f"Failed to open FSUIPC connection")
    sleep(5)

res = fsuipc_client.get_connection_state()
print(f"FSUIPC connection state: {res.status}")

while True:
    res = fsuipc_client.get_frequency()
    print(res.frequency)
    sleep(1)

```

## License

MIT License

Copyright © 2025 Half_nothing

No additional terms
