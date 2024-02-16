#pragma once

#include <winerror.h>
#include <iostream>
#include <assert.h>
#include <filesystem>


#define DX_CHECK(f)                                                                                  \
    {                                                                                                \
        HRESULT res = (f);                                                                           \
        if (!SUCCEEDED(res)) {                                                                       \
            printf("[NoAovsInteropApp.h] Fatal : HRESULT is %d in %s at line %d\n", res, __FILE__, __LINE__); \
            _com_error err(res);                                                                     \
            printf("[NoAovsInteropApp.h] messsage : %s\n", err.ErrorMessage());                               \
            assert(false);                                                                           \
        }                                                                                            \
    }

struct GpuIndices {
    int dx11 = 0;
    int vk = 0;
};

struct Paths {
    std::filesystem::path hybridproDll;
    std::filesystem::path hybridproCacheDir;
    std::filesystem::path assetsDir;
};