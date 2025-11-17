#pragma once
#include <stdint.h>
#ifdef _WIN32
#define BOTDRIVER_API __declspec(dllexport)
#else
#define BOTDRIVER_API
#endif
extern "C" {
BOTDRIVER_API void* BD_Create();
BOTDRIVER_API void BD_Destroy(void* h);
BOTDRIVER_API int BD_Init(void* h);
BOTDRIVER_API void BD_Exit(void* h);
BOTDRIVER_API int BD_Open(void* h, uint16_t vid, uint16_t pid);
BOTDRIVER_API void BD_Close(void* h);
BOTDRIVER_API int BD_ProbeEndpoints(void* h);
BOTDRIVER_API int BD_Write(void* h, const uint8_t* data, int length, int timeout_ms, int* transferred);
BOTDRIVER_API int BD_Read(void* h, uint8_t* data, int length, int timeout_ms, int* transferred);
BOTDRIVER_API char* BD_ListDevices();
BOTDRIVER_API void BD_Free(void* p);
}