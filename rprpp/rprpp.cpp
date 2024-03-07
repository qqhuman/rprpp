#include "rprpp.h"

#include "Context.h"
#include "Error.h"
#include "PostProcessing.h"
#include "vk/DeviceContext.h"

#include <cassert>
#include <concepts>
#include <functional>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#include <iostream>

static std::mutex Mutex;
static map<rprpp::Context> GlobalContextObjects;

template <class Function,
    class... Params>
[[nodiscard("Please, don't ignore result")]] inline auto safeCall(Function function, Params&&... params) noexcept -> std::expected<std::invoke_result_t<Function, Params&&...>, RprPpError>
{
    try {
        if constexpr (!std::is_same<std::invoke_result_t<Function, Params&&...>, void>::value) {
            return std::invoke(function, std::forward<Params>(params)...);
        } else {
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

    auto result = safeCall(rprpp::Context::create, deviceId);
    check(result);

    rprpp::Context* context = result->get();
    *outContext = context;

    // avoid memleak
    std::lock_guard<std::mutex> lock(Mutex);
    GlobalContextObjects.emplace(context, std::move(*result));

    return RPRPP_SUCCESS;
}

RprPpError rprppDestroyContext(RprPpContext context)
{
    if (!context) {
        std::cerr << "[WARING] context in null, but it should never be";
        return RPRPP_SUCCESS;
    }

    std::lock_guard<std::mutex> lock(Mutex);
    auto result = safeCall([context]() {
        GlobalContextObjects.erase(static_cast<rprpp::Context*>(context));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCreatePostProcessing(RprPpContext context, RprPpPostProcessing* outpp)
{
    assert(context);
    assert(outpp);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        *outpp = ctx->createPostProcessing();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextDestroyPostProcessing(RprPpContext context, RprPpPostProcessing pp)
{
    assert(context);
    assert(pp);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        ctx->destroyPostProcessing(static_cast<rprpp::PostProcessing*>(pp));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCreateBuffer(RprPpContext context, size_t size, RprPpBuffer* outBuffer)
{
    assert(context);
    assert(outBuffer);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        *outBuffer = ctx->createBuffer(size);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextDestroyBuffer(RprPpContext context, RprPpBuffer buffer)
{
    assert(context);
    assert(buffer);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        ctx->destroyBuffer(static_cast<rprpp::Buffer*>(buffer));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCreateImage(RprPpContext context, RprPpImageDescription description, RprPpImage* outImage)
{
    assert(context);
    assert(outImage);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        *outImage = ctx->createImage(rprpp::ImageDescription(description));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCreateImageFromVkSampledImage(RprPpContext context, RprPpVkImage vkSampledImage, RprPpImageDescription description, RprPpImage* outImage)
{
    assert(context);
    assert(outImage);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        *outImage = ctx->createFromVkSampledImage(static_cast<vk::Image>(static_cast<VkImage>(vkSampledImage)), rprpp::ImageDescription(description));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCreateImageFromDx11Texture(RprPpContext context, RprPpDx11Handle dx11textureHandle, RprPpImageDescription description, RprPpImage* outImage)
{
    assert(context);
    assert(outImage);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        *outImage = ctx->createImageFromDx11Texture(dx11textureHandle, rprpp::ImageDescription(description));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextDestroyImage(RprPpContext context, RprPpImage image)
{
    assert(context);
    assert(image);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        ctx->destroyImage(static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyBufferToImage(RprPpContext context, RprPpBuffer buffer, RprPpImage image)
{
    assert(context);
    assert(image);
    assert(buffer);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        ctx->copyBufferToImage(static_cast<rprpp::Buffer*>(buffer), static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyImageToBuffer(RprPpContext context, RprPpImage image, RprPpBuffer buffer)
{
    assert(context);
    assert(image);
    assert(buffer);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        ctx->copyImageToBuffer(static_cast<rprpp::Image*>(image), static_cast<rprpp::Buffer*>(buffer));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextCopyImage(RprPpContext context, RprPpImage src, RprPpImage dst)
{
    assert(context);
    assert(src);
    assert(dst);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        ctx->copyImage(static_cast<rprpp::Image*>(src), static_cast<rprpp::Image*>(dst));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextGetVkPhysicalDevice(RprPpContext context, RprPpVkPhysicalDevice* physicalDevice)
{
    assert(context);
    assert(physicalDevice);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        *physicalDevice = ctx->getVkPhysicalDevice();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextGetVkDevice(RprPpContext context, RprPpVkDevice* device)
{
    assert(context);
    assert(device);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        *device = ctx->getVkDevice();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextGetVkQueue(RprPpContext context, RprPpVkQueue* queue)
{
    assert(context);
    assert(queue);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        *queue = ctx->getVkQueue();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppContextWaitQueueIdle(RprPpContext context)
{
    assert(context);

    auto result = safeCall([&] {
        rprpp::Context* ctx = static_cast<rprpp::Context*>(context);
        ctx->waitQueueIdle();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingRun(RprPpPostProcessing processing, RprPpVkSemaphore inAovsReadySemaphore, RprPpVkSemaphore inToSignalAfterProcessingSemaphore)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        std::optional<vk::Semaphore> aovsReadySemaphore;
        if (inAovsReadySemaphore != nullptr) {
            aovsReadySemaphore = static_cast<vk::Semaphore>(static_cast<VkSemaphore>(inAovsReadySemaphore));
        }

        std::optional<vk::Semaphore> toSignalAfterProcessingSemaphore;
        if (inToSignalAfterProcessingSemaphore != nullptr) {
            toSignalAfterProcessingSemaphore = static_cast<vk::Semaphore>(static_cast<VkSemaphore>(inToSignalAfterProcessingSemaphore));
        }

        pp->run(aovsReadySemaphore, toSignalAfterProcessingSemaphore);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetOutput(RprPpPostProcessing processing, RprPpImage image)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setOutput(static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetAovColor(RprPpPostProcessing processing, RprPpImage image)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setAovColor(static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetAovOpacity(RprPpPostProcessing processing, RprPpImage image)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setAovOpacity(static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetAovShadowCatcher(RprPpPostProcessing processing, RprPpImage image)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setAovShadowCatcher(static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetAovReflectionCatcher(RprPpPostProcessing processing, RprPpImage image)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setAovReflectionCatcher(static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetAovMattePass(RprPpPostProcessing processing, RprPpImage image)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setAovMattePass(static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetAovBackground(RprPpPostProcessing processing, RprPpImage image)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setAovBackground(static_cast<rprpp::Image*>(image));
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapWhitepoint(RprPpPostProcessing processing, float x, float y, float z)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapWhitepoint(x, y, z);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapVignetting(RprPpPostProcessing processing, float vignetting)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapVignetting(vignetting);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapCrushBlacks(RprPpPostProcessing processing, float crushBlacks)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapCrushBlacks(crushBlacks);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapBurnHighlights(RprPpPostProcessing processing, float burnHighlights)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapBurnHighlights(burnHighlights);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapSaturation(RprPpPostProcessing processing, float saturation)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapSaturation(saturation);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapCm2Factor(RprPpPostProcessing processing, float cm2Factor)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapCm2Factor(cm2Factor);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapFilmIso(RprPpPostProcessing processing, float filmIso)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapFilmIso(filmIso);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapCameraShutter(RprPpPostProcessing processing, float cameraShutter)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapCameraShutter(cameraShutter);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapFNumber(RprPpPostProcessing processing, float fNumber)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapFNumber(fNumber);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapFocalLength(RprPpPostProcessing processing, float focalLength)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapFocalLength(focalLength);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetToneMapAperture(RprPpPostProcessing processing, float aperture)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setToneMapAperture(aperture);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetBloomRadius(RprPpPostProcessing processing, float radius)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setBloomRadius(radius);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetBloomBrightnessScale(RprPpPostProcessing processing, float brightnessScale)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setBloomBrightnessScale(brightnessScale);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetBloomThreshold(RprPpPostProcessing processing, float threshold)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setBloomThreshold(threshold);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetBloomEnabled(RprPpPostProcessing processing, RprPpBool enabled)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setBloomEnabled(RPRPP_TRUE == enabled);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetTileOffset(RprPpPostProcessing processing, uint32_t x, uint32_t y)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setTileOffset(x, y);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetGamma(RprPpPostProcessing processing, float gamma)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setGamma(gamma);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetShadowIntensity(RprPpPostProcessing processing, float shadowIntensity)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setShadowIntensity(shadowIntensity);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingSetDenoiserEnabled(RprPpPostProcessing processing, RprPpBool enabled)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        pp->setDenoiserEnabled(RPRPP_TRUE == enabled);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapWhitepoint(RprPpPostProcessing processing, float* x, float* y, float* z)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        float whitepoint[3];
        pp->getToneMapWhitepoint(whitepoint[0], whitepoint[1], whitepoint[2]);

        if (x != nullptr) {
            *x = whitepoint[0];
        }

        if (y != nullptr) {
            *y = whitepoint[1];
        }

        if (z != nullptr) {
            *z = whitepoint[2];
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapVignetting(RprPpPostProcessing processing, float* vignetting)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (vignetting != nullptr) {
            *vignetting = pp->getToneMapVignetting();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapCrushBlacks(RprPpPostProcessing processing, float* crushBlacks)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (crushBlacks != nullptr) {
            *crushBlacks = pp->getToneMapCrushBlacks();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapBurnHighlights(RprPpPostProcessing processing, float* burnHighlights)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (burnHighlights != nullptr) {
            *burnHighlights = pp->getToneMapBurnHighlights();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapSaturation(RprPpPostProcessing processing, float* saturation)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (saturation != nullptr) {
            *saturation = pp->getToneMapSaturation();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapCm2Factor(RprPpPostProcessing processing, float* cm2Factor)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (cm2Factor != nullptr) {
            *cm2Factor = pp->getToneMapCm2Factor();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapFilmIso(RprPpPostProcessing processing, float* filmIso)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (filmIso != nullptr) {
            *filmIso = pp->getToneMapFilmIso();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapCameraShutter(RprPpPostProcessing processing, float* cameraShutter)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (cameraShutter != nullptr) {
            *cameraShutter = pp->getToneMapCameraShutter();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapFNumber(RprPpPostProcessing processing, float* fNumber)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (fNumber != nullptr) {
            *fNumber = pp->getToneMapFNumber();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapFocalLength(RprPpPostProcessing processing, float* focalLength)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (focalLength != nullptr) {
            *focalLength = pp->getToneMapFocalLength();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetToneMapAperture(RprPpPostProcessing processing, float* aperture)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (aperture != nullptr) {
            *aperture = pp->getToneMapAperture();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetBloomRadius(RprPpPostProcessing processing, float* radius)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (radius != nullptr) {
            *radius = pp->getBloomRadius();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetBloomBrightnessScale(RprPpPostProcessing processing, float* brightnessScale)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (brightnessScale != nullptr) {
            *brightnessScale = pp->getBloomBrightnessScale();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetBloomThreshold(RprPpPostProcessing processing, float* threshold)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (threshold != nullptr) {
            *threshold = pp->getBloomThreshold();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetBloomEnabled(RprPpPostProcessing processing, RprPpBool* enabled)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (enabled != nullptr) {
            *enabled = pp->getBloomEnabled() ? RPRPP_TRUE : RPRPP_FALSE;
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetTileOffset(RprPpPostProcessing processing, uint32_t* x, uint32_t* y)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);
        uint32_t xy[2];
        pp->getTileOffset(xy[0], xy[1]);

        if (x != nullptr) {
            *x = xy[0];
        }

        if (y != nullptr) {
            *y = xy[1];
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetGamma(RprPpPostProcessing processing, float* gamma)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (gamma != nullptr) {
            *gamma = pp->getGamma();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetShadowIntensity(RprPpPostProcessing processing, float* shadowIntensity)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (shadowIntensity != nullptr) {
            *shadowIntensity = pp->getShadowIntensity();
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppPostProcessingGetDenoiserEnabled(RprPpPostProcessing processing, RprPpBool* enabled)
{
    assert(processing);

    auto result = safeCall([&] {
        rprpp::PostProcessing* pp = static_cast<rprpp::PostProcessing*>(processing);

        if (enabled != nullptr) {
            *enabled = pp->getDenoiserEnabled() ? RPRPP_TRUE : RPRPP_FALSE;
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppBufferMap(RprPpBuffer buffer, size_t size, void** outdata)
{
    assert(buffer);

    auto result = safeCall([&] {
        rprpp::Buffer* buff = static_cast<rprpp::Buffer*>(buffer);

        if (outdata != nullptr) {
            *outdata = buff->map(size);
        }
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppBufferUnmap(RprPpBuffer buffer)
{
    assert(buffer);

    auto result = safeCall([&] {
        rprpp::Buffer* buff = static_cast<rprpp::Buffer*>(buffer);
        buff->unmap();
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppVkCreateSemaphore(RprPpVkDevice device, RprPpVkSemaphore* outSemaphore)
{
    assert(device);

    auto result = safeCall([&] {
        vk::Device vkdevice = static_cast<VkDevice>(device);
        *outSemaphore = (VkSemaphore)vkdevice.createSemaphore({});
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppVkDestroySemaphore(RprPpVkDevice device, RprPpVkSemaphore semaphore)
{
    assert(device);

    auto result = safeCall([&] {
        vk::Device vkdevice = static_cast<VkDevice>(device);
        vk::Semaphore vksemaphore = static_cast<VkSemaphore>(semaphore);
        vkdevice.destroySemaphore(vksemaphore);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppVkCreateFence(RprPpVkDevice device, RprPpBool signaled, RprPpVkFence* outFence)
{
    assert(device);

    auto result = safeCall([&] {
        vk::Device vkdevice = static_cast<VkDevice>(device);
        vk::FenceCreateInfo info;
        if (signaled == RPRPP_TRUE) {
            info.flags = vk::FenceCreateFlagBits::eSignaled;
        }
        *outFence = (VkFence)vkdevice.createFence(info);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppVkDestroyFence(RprPpVkDevice device, RprPpVkFence fence)
{
    assert(device);

    auto result = safeCall([&] {
        vk::Device vkdevice = static_cast<VkDevice>(device);
        vk::Fence vkfence = static_cast<VkFence>(fence);
        vkdevice.destroyFence(vkfence);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppVkWaitForFences(RprPpVkDevice device, uint32_t fenceCount, RprPpVkFence* pFences, RprPpBool waitAll, uint64_t timeout)
{
    assert(device);

    auto result = safeCall([&] {
        VkDevice vkdevice = static_cast<VkDevice>(device);
        VkFence* vkfences = reinterpret_cast<VkFence*>(pFences);
        vkWaitForFences(vkdevice, fenceCount, vkfences, waitAll, timeout);
    });
    check(result);

    return RPRPP_SUCCESS;
}
RprPpError rprppVkResetFences(RprPpVkDevice device, uint32_t fenceCount, RprPpVkFence* pFences)
{
    assert(device);

    auto result = safeCall([&] {
        VkDevice vkdevice = static_cast<VkDevice>(device);
        VkFence* vkfences = reinterpret_cast<VkFence*>(pFences);
        vkResetFences(vkdevice, fenceCount, vkfences);
    });
    check(result);

    return RPRPP_SUCCESS;
}

RprPpError rprppVkQueueSubmitWaitAndSignal(RprPpVkQueue queue, RprPpVkSemaphore waitSemaphore, RprPpVkSemaphore signalSemaphore, RprPpVkFence fence)
{
    assert(queue);

    auto result = safeCall([&] {
        VkQueue vkqueue = static_cast<VkQueue>(queue);
        VkFence vkfence = static_cast<VkFence>(fence);
        VkSemaphore vkwaitSemaphore = static_cast<VkSemaphore>(waitSemaphore);
        VkSemaphore vksignalSemaphore = static_cast<VkSemaphore>(signalSemaphore);

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        if (vkwaitSemaphore != VK_NULL_HANDLE) {
            submitInfo.pWaitDstStageMask = &waitStage;
            submitInfo.pWaitSemaphores = &vkwaitSemaphore;
            submitInfo.waitSemaphoreCount = 1;
        }

        if (vksignalSemaphore != VK_NULL_HANDLE) {
            submitInfo.pSignalSemaphores = &vksignalSemaphore;
            submitInfo.signalSemaphoreCount = 1;
        }

        vkQueueSubmit(vkqueue, 1, &submitInfo, vkfence);
    });
    check(result);

    return RPRPP_SUCCESS;
}
