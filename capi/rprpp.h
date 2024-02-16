#ifndef __RPRPP_H
#define __RPRPP_H

#include <stdint.h>

#ifdef _WIN32
#ifdef RPRPP_EXPORT_API
#define RPRPP_API __declspec(dllexport)
#else
#define RPRPP_API __declspec(dllimport)
#endif
#else
#define RPRPP_API __attribute__((visibility("default")))
#endif

#define RPRPP_TRUE 1u
#define RPRPP_FALSE 0u

typedef enum RprPpError {
    RPRPP_SUCCESS = 0,
    RPRPP_ERROR_INTERNAL_ERROR = 1,
    RPRPP_ERROR_INVALID_DEVICE = 2,
    RPRPP_ERROR_INVALID_PARAMETER = 3,
    RPRPP_ERROR_SHADER_COMPILATION = 4,
    RPRPP_ERROR_INVALID_OPERATION = 5,
} RprPpError;

typedef enum RprPpDeviceInfo {
    RPRPP_DEVICE_INFO_NAME = 0,
} RprPpDeviceInfo;

typedef enum RprPpImageFormat {
    RPRPP_IMAGE_FROMAT_R8G8B8A8_UNORM = 0,
    RPRPP_IMAGE_FROMAT_R32G32B32A32_SFLOAT = 1,
} RprPpImageFormat;

typedef void* RprPpContext;
typedef void* RprPpDx11Handle;
typedef void* RprPpVkSemaphore;
typedef void* RprPpVkPhysicalDevice;
typedef void* RprPpVkDevice;
typedef void* RprPpVkQueue;
typedef void* RprPpVkImage;

typedef struct RprPpAovsVkInteropInfo {
    RprPpVkImage color;
    RprPpVkImage opacity;
    RprPpVkImage shadowCatcher;
    RprPpVkImage reflectionCatcher;
    RprPpVkImage mattePass;
    RprPpVkImage background;
} RprPpVkInteropAovs;

#ifdef __cplusplus
extern "C" {
#endif

RPRPP_API RprPpError rprppGetDeviceCount(uint32_t* deviceCount);
RPRPP_API RprPpError rprppGetDeviceInfo(uint32_t deviceId, RprPpDeviceInfo deviceInfo, void* data, size_t size, size_t* sizeRet);

RPRPP_API RprPpError rprppCreateContext(uint32_t deviceId, RprPpContext* outContext);
RPRPP_API RprPpError rprppDestroyContext(RprPpContext context);

RPRPP_API RprPpError rprppContextGetOutput(RprPpContext context, uint8_t* dst, size_t size, size_t* retSize);
RPRPP_API RprPpError rprppContextGetVkPhysicalDevice(RprPpContext context, RprPpVkPhysicalDevice* physicalDevice);
RPRPP_API RprPpError rprppContextGetVkDevice(RprPpContext context, RprPpVkDevice* device);
RPRPP_API RprPpError rprppContextGetVkQueue(RprPpContext context, RprPpVkQueue* queue);
RPRPP_API RprPpError rprppContextSetFramesInFlihgt(RprPpContext context, uint32_t framesInFlight);
RPRPP_API RprPpError rprppContextResize(RprPpContext context, uint32_t width, uint32_t height, RprPpImageFormat format, RprPpDx11Handle outputDx11TextureHandle, RprPpAovsVkInteropInfo* aovsVkInteropInfo);
RPRPP_API RprPpError rprppContextRun(RprPpContext context, RprPpVkSemaphore aovsReadySemaphore, RprPpVkSemaphore toSignalAfterProcessingSemaphore);
RPRPP_API RprPpError rprppContextWaitQueueIdle(RprPpContext context);

RPRPP_API RprPpError rprppContextMapStagingBuffer(RprPpContext context, size_t size, void** data);
RPRPP_API RprPpError rprppContextUnmapStagingBuffer(RprPpContext context);
RPRPP_API RprPpError rprppContextCopyStagingBufferToAovColor(RprPpContext context);
RPRPP_API RprPpError rprppContextCopyStagingBufferToAovOpacity(RprPpContext context);
RPRPP_API RprPpError rprppContextCopyStagingBufferToAovShadowCatcher(RprPpContext context);
RPRPP_API RprPpError rprppContextCopyStagingBufferToAovReflectionCatcher(RprPpContext context);
RPRPP_API RprPpError rprppContextCopyStagingBufferToAovMattePass(RprPpContext context);
RPRPP_API RprPpError rprppContextCopyStagingBufferToAovBackground(RprPpContext context);

RPRPP_API RprPpError rprppContextSetToneMapWhitepoint(RprPpContext context, float x, float y, float z);
RPRPP_API RprPpError rprppContextSetToneMapVignetting(RprPpContext context, float vignetting);
RPRPP_API RprPpError rprppContextSetToneMapCrushBlacks(RprPpContext context, float crushBlacks);
RPRPP_API RprPpError rprppContextSetToneMapBurnHighlights(RprPpContext context, float burnHighlights);
RPRPP_API RprPpError rprppContextSetToneMapSaturation(RprPpContext context, float saturation);
RPRPP_API RprPpError rprppContextSetToneMapCm2Factor(RprPpContext context, float cm2Factor);
RPRPP_API RprPpError rprppContextSetToneMapFilmIso(RprPpContext context, float filmIso);
RPRPP_API RprPpError rprppContextSetToneMapCameraShutter(RprPpContext context, float cameraShutter);
RPRPP_API RprPpError rprppContextSetToneMapFNumber(RprPpContext context, float fNumber);
RPRPP_API RprPpError rprppContextSetToneMapFocalLength(RprPpContext context, float focalLength);
RPRPP_API RprPpError rprppContextSetToneMapAperture(RprPpContext context, float aperture);
RPRPP_API RprPpError rprppContextSetBloomRadius(RprPpContext context, float radius);
RPRPP_API RprPpError rprppContextSetBloomBrightnessScale(RprPpContext context, float brightnessScale);
RPRPP_API RprPpError rprppContextSetBloomThreshold(RprPpContext context, float threshold);
RPRPP_API RprPpError rprppContextSetBloomEnabled(RprPpContext context, uint32_t enabled);
RPRPP_API RprPpError rprppContextSetGamma(RprPpContext context, float gamma);
RPRPP_API RprPpError rprppContextSetShadowIntensity(RprPpContext context, float shadowIntensity);
RPRPP_API RprPpError rprppContextSetDenoiserEnabled(RprPpContext context, uint32_t enabled);

#ifdef __cplusplus
}
#endif

#endif
