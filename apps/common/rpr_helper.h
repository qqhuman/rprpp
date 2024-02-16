#pragma once

#include <RadeonProRender.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "capi/rprpp.h"

#define VK_CHECK(x)                              \
    {                                            \
        if ((x) != VK_SUCCESS) {                 \
            ErrorManager(x, __FILE__, __LINE__); \
        }                                        \
    }

#define RPR_CHECK(x)                             \
    {                                            \
        if ((x) != RPR_SUCCESS) {                \
            ErrorManager(x, __FILE__, __LINE__); \
        }                                        \
    }

#define RPRPP_CHECK(x)                           \
    {                                            \
        if ((x) != RPRPP_SUCCESS) {              \
            ErrorManager(x, __FILE__, __LINE__); \
        }                                        \
    }

inline rpr_creation_flags intToRprCreationFlag(int index)
{
    switch (index) {
    case 1:
        return RPR_CREATION_FLAGS_ENABLE_GPU1;
    case 2:
        return RPR_CREATION_FLAGS_ENABLE_GPU2;
    case 3:
        return RPR_CREATION_FLAGS_ENABLE_GPU3;
    case 4:
        return RPR_CREATION_FLAGS_ENABLE_GPU4;
    case 5:
        return RPR_CREATION_FLAGS_ENABLE_GPU5;
    case 6:
        return RPR_CREATION_FLAGS_ENABLE_GPU6;
    case 7:
        return RPR_CREATION_FLAGS_ENABLE_GPU7;
    case 8:
        return RPR_CREATION_FLAGS_ENABLE_GPU8;
    case 9:
        return RPR_CREATION_FLAGS_ENABLE_GPU9;
    case 10:
        return RPR_CREATION_FLAGS_ENABLE_GPU10;
    case 11:
        return RPR_CREATION_FLAGS_ENABLE_GPU11;
    case 12:
        return RPR_CREATION_FLAGS_ENABLE_GPU12;
    case 13:
        return RPR_CREATION_FLAGS_ENABLE_GPU13;
    case 14:
        return RPR_CREATION_FLAGS_ENABLE_GPU14;
    case 15:
        return RPR_CREATION_FLAGS_ENABLE_GPU15;
    case 0:
    default:
        return RPR_CREATION_FLAGS_ENABLE_GPU0;
        break;
    }
}

void ErrorManager(VkResult result, const char* fileName, int line);
void ErrorManager(rpr_status errorCode, const char* fileName, int line);
void ErrorManager(RprPpError errorCode, const char* fileName, int line);
void CheckNoLeak(rpr_context context);
rpr_shape ImportOBJ(const std::string& file, rpr_context ctx);