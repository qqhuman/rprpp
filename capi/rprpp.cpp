#include "rprpp.h"

#include "Error.h"
#include "PostProcessing.h"
#include "vk_helper.h"

#include <utility>
#include <type_traits>
#include <concepts>
#include <cassert>
#include <functional>
#include <optional>

#include <iostream>

template <class Function, 
    class... Params>
[[nodiscard("Please, don't ignore result")]]
inline auto safeCall(Function function, Params&&... params) noexcept -> std::expected<std::invoke_result_t<Function, Params&&...>, RprPpError>
{
    try {   
        if constexpr (!std::is_same<std::invoke_result_t<Function, Params&&...>, void>::value)
        {
            return std::invoke(function, std::forward<Params>(params)...);
        } 
        else 
        {
            std::invoke(function, std::forward<Params>(params)...);
            return {};
        }
    } catch (const rprpp::Error& e) {
        std::cerr << e.what();
        return std::unexpected(static_cast<RprPpError>(e.getErrorCode()));
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return std::unexpected(RPRPP_ERROR_INTERNAL_ERROR);
    } catch (...) {
        std::cerr << "unkown error";
        return std::unexpected(RPRPP_ERROR_INTERNAL_ERROR);
    }
}

#define check(status)        \
    if (!status.has_value()) \
    return status.error();

RprPpError rprppGetDeviceCount(uint32_t* deviceCount)
{
    auto result = safeCall(vk::helper::getDeviceCount);
    check(result);
    
    *deviceCount = *result;

    return RPRPP_SUCCESS;
}

RprPpError rprppGetDeviceInfo(uint32_t deviceId, RprPpDeviceInfo deviceInfo, void* data, size_t size, size_t* sizeRet)
{
    auto result = safeCall(vk::helper::getDeviceInfo, deviceId, static_cast<vk::helper::DeviceInfo>(deviceInfo), data, size, sizeRet);
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppCreateContext(uint32_t deviceId, RprPpContext* outContext)
{
	assert(outContext);

    auto result = safeCall(rprpp::PostProcessing::create, deviceId);
    check(result);
    
    *outContext = *result;

    return RPRPP_SUCCESS;
}

RprPpError rprppDestroyContext(RprPpContext context)
{
	if (!context) 
		return RPRPP_SUCCESS;

	rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
	delete pp;

    return RPRPP_SUCCESS;
}

RprPpError rprppContextGetOutput(RprPpContext context, uint8_t* dst, size_t size, size_t* retSize)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
	    pp->getOutput(dst, size, retSize);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextGetVkPhysicalDevice(RprPpContext context, RprPpVkPhysicalDevice* physicalDevice)
{
    assert(context);
    assert(physicalDevice);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        *physicalDevice = pp->getVkPhysicalDevice();
    });
    check(result);

	return RPRPP_SUCCESS;
}

RprPpError rprppContextGetVkDevice(RprPpContext context, RprPpVkDevice* device)
{
	assert(context);
	assert(device);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        *device = pp->getVkDevice();
    });
    check(result);
    
    return RPRPP_SUCCESS;
}

RprPpError rprppContextGetVkQueue(RprPpContext context, RprPpVkQueue* queue)
{
	assert(context);
	assert(queue);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        *queue = pp->getVkQueue();
    });
    check(result);
    
    return RPRPP_SUCCESS;
}

RPRPP_API RprPpError rprppContextSetFramesInFlihgt(RprPpContext context, uint32_t framesInFlight)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setFramesInFlihgt(framesInFlight);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextResize(RprPpContext context, uint32_t width, uint32_t height, RprPpImageFormat format, RprPpDx11Handle outputDx11TextureHandle, RprPpAovsVkInteropInfo* pAovsVkInterop)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);

        std::optional<rprpp::AovsVkInteropInfo> aovsVkInteropInfo;
        if (pAovsVkInterop != nullptr) {
            aovsVkInteropInfo = {
                .color = static_cast<VkImage>(pAovsVkInterop->color),
                .opacity = static_cast<VkImage>(pAovsVkInterop->opacity),
                .shadowCatcher = static_cast<VkImage>(pAovsVkInterop->shadowCatcher),
                .reflectionCatcher = static_cast<VkImage>(pAovsVkInterop->reflectionCatcher),
                .mattePass = static_cast<VkImage>(pAovsVkInterop->mattePass),
                .background = static_cast<VkImage>(pAovsVkInterop->background),
            };
        }

        pp->resize(width, height, static_cast<rprpp::ImageFormat>(format), outputDx11TextureHandle, aovsVkInteropInfo);
    });
    check(result);

   return RPRPP_SUCCESS;
}

RprPpError rprppContextRun(RprPpContext context, RprPpVkSemaphore aovsReadySemaphore, RprPpVkSemaphore toSignalAfterProcessingSemaphore)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->run(static_cast<VkSemaphore>(aovsReadySemaphore), static_cast<VkSemaphore>(toSignalAfterProcessingSemaphore));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextWaitQueueIdle(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->waitQueueIdle();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextMapStagingBuffer(RprPpContext context, size_t size, void** data)
{
    assert(context);
    assert(data);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        *data = pp->mapStagingBuffer(size);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextUnmapStagingBuffer(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->unmapStagingBuffer();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyStagingBufferToAovColor(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->copyStagingBufferToAovColor();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyStagingBufferToAovOpacity(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->copyStagingBufferToAovOpacity();
    });
    check(result);
   
    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyStagingBufferToAovShadowCatcher(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->copyStagingBufferToAovShadowCatcher();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyStagingBufferToAovReflectionCatcher(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->copyStagingBufferToAovReflectionCatcher();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyStagingBufferToAovMattePass(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->copyStagingBufferToAovMattePass();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyStagingBufferToAovBackground(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->copyStagingBufferToAovBackground();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapWhitepoint(RprPpContext context, float x, float y, float z)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapWhitepoint(x, y, z);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapVignetting(RprPpContext context, float vignetting)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapVignetting(vignetting);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapCrushBlacks(RprPpContext context, float crushBlacks)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapCrushBlacks(crushBlacks);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapBurnHighlights(RprPpContext context, float burnHighlights)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapBurnHighlights(burnHighlights);
    });
    check(result);
    
    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapSaturation(RprPpContext context, float saturation)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapSaturation(saturation);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapCm2Factor(RprPpContext context, float cm2Factor)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapCm2Factor(cm2Factor);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapFilmIso(RprPpContext context, float filmIso)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapFilmIso(filmIso);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapCameraShutter(RprPpContext context, float cameraShutter)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapCameraShutter(cameraShutter);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapFNumber(RprPpContext context, float fNumber)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapFNumber(fNumber);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapFocalLength(RprPpContext context, float focalLength)
{
    assert(context);
	
    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapFocalLength(focalLength);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetToneMapAperture(RprPpContext context, float aperture)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setToneMapAperture(aperture);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetBloomRadius(RprPpContext context, float radius)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setBloomRadius(radius);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetBloomBrightnessScale(RprPpContext context, float brightnessScale)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = (rprpp::PostProcessing*)context;
        pp->setBloomBrightnessScale(brightnessScale);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetBloomThreshold(RprPpContext context, float threshold)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setBloomThreshold(threshold);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetBloomEnabled(RprPpContext context, uint32_t enabled)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setBloomEnabled(RPRPP_TRUE == enabled);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetGamma(RprPpContext context, float gamma)
{
    assert(context);
	
    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setGamma(gamma);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetShadowIntensity(RprPpContext context, float shadowIntensity)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setShadowIntensity(shadowIntensity);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextSetDenoiserEnabled(RprPpContext context, uint32_t enabled)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(context);
        pp->setDenoiserEnabled(RPRPP_TRUE == enabled);
    });
    check(result);

	return RPRPP_SUCCESS;
}
