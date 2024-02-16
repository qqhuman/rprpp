#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "common/HybridProRenderer.h"
#include "common/RprPostProcessing.h"
#include <filesystem>
#include <iostream>
#include <unordered_map>

#define WIDTH 1200
#define HEIGHT 700
#define INTEROP false
#define DEVICE_ID 0
// please note that when we use frames in flight > 1 
// hybridpro produces Validation Error with VK_OBJECT_TYPE_QUERY_POOL message looks like "query not reset. After query pool creation"
#define FRAMES_IN_FLIGHT 1
#define ITERATIONS 100

void savePngImage(const char* filename, uint8_t* img, uint32_t width, uint32_t height, RprPpImageFormat format);
void copyRprFbToPpStagingBuffer(HybridProRenderer& r, RprPostProcessing& pp, rpr_aov aov);
void runWithInterop(const std::filesystem::path& exeDirPath, int device_id);
void runWithoutInterop(const std::filesystem::path& exeDirPath, int device_id);

int main(int argc, const char* argv[])
{
    std::cout << "ConsoleApp started..." << std::endl;
    try {
        uint32_t deviceCount;
        RPRPP_CHECK(rprppGetDeviceCount(&deviceCount));
        for (size_t i = 0; i < deviceCount; i++) {
            size_t size;
            RPRPP_CHECK(rprppGetDeviceInfo(i, RPRPP_DEVICE_INFO_NAME, nullptr, 0, &size));
            std::vector<char> deviceName;
            deviceName.resize(size);
            RPRPP_CHECK(rprppGetDeviceInfo(i, RPRPP_DEVICE_INFO_NAME, deviceName.data(), size, nullptr));
            std::cout << "Device id = " << i << ", name = " << std::string(deviceName.begin(), deviceName.end()) << std::endl;
        }

        std::filesystem::path exeDirPath = std::filesystem::path(argv[0]).parent_path();
        if (INTEROP) {
            runWithInterop(exeDirPath, DEVICE_ID);
        } else {
            runWithoutInterop(exeDirPath, DEVICE_ID);
        }

    } catch (const std::runtime_error& e) {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }
    std::cout << "ConsoleApp finished..." << std::endl;
    return 0;
}

void runWithInterop(const std::filesystem::path& exeDirPath, int deviceId)
{
    std::filesystem::path hybridproDll = exeDirPath / "HybridPro.dll";
    std::filesystem::path hybridproCacheDir = exeDirPath / "hybridpro_cache";
    std::filesystem::path assetsDir = exeDirPath;

    RprPpImageFormat format = RPRPP_IMAGE_FROMAT_R32G32B32A32_SFLOAT;
    RprPostProcessing postProcessing(deviceId);

    VkPhysicalDevice physicalDevice = (VkPhysicalDevice)postProcessing.getVkPhysicalDevice();
    VkDevice device = (VkDevice)postProcessing.getVkDevice();
    VkQueue queue = (VkQueue)postProcessing.getVkQueue();
    std::vector<VkFence> fences;
    std::vector<VkSemaphore> frameBuffersReleaseSemaphores;
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkSemaphore semaphore;
        VkSemaphoreCreateInfo semaphoreInfo { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
        frameBuffersReleaseSemaphores.push_back(semaphore);

        VkFence fence;
        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &fence));
        fences.push_back(fence);
    }

    // set frame buffers realese to signal state
    {
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pSignalSemaphores = frameBuffersReleaseSemaphores.data();
        submitInfo.signalSemaphoreCount = frameBuffersReleaseSemaphores.size();
        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, nullptr));
    }

    HybridProInteropInfo aovsInteropInfo = HybridProInteropInfo {
        .physicalDevice = physicalDevice,
        .device = device,
        .framesInFlight = FRAMES_IN_FLIGHT,
        .frameBuffersReleaseSemaphores = frameBuffersReleaseSemaphores.data(),
    };

    HybridProRenderer renderer(deviceId, aovsInteropInfo, hybridproDll, hybridproCacheDir, assetsDir);
    auto frameBuffersReadySemaphores = renderer.getFrameBuffersReadySemaphores();

    renderer.resize(WIDTH, HEIGHT);
    RprPpAovsVkInteropInfo aovsVkInteropInfo = {
        .color = (RprPpVkImage)renderer.getAovVkImage(RPR_AOV_COLOR),
        .opacity = (RprPpVkImage)renderer.getAovVkImage(RPR_AOV_OPACITY),
        .shadowCatcher = (RprPpVkImage)renderer.getAovVkImage(RPR_AOV_SHADOW_CATCHER),
        .reflectionCatcher = (RprPpVkImage)renderer.getAovVkImage(RPR_AOV_REFLECTION_CATCHER),
        .mattePass = (RprPpVkImage)renderer.getAovVkImage(RPR_AOV_MATTE_PASS),
        .background = (RprPpVkImage)renderer.getAovVkImage(RPR_AOV_BACKGROUND),
    };
    postProcessing.resize(WIDTH, HEIGHT, format, nullptr, &aovsVkInteropInfo);
    postProcessing.setFramesInFlihgt(FRAMES_IN_FLIGHT);

    std::vector<uint8_t> output;
    uint32_t currentFrame = 0;
    for (size_t i = 0; i < ITERATIONS; i++) {
        renderer.render();
        renderer.flushFrameBuffers();
        
        VkFence fence = fences[currentFrame];
        VK_CHECK(vkWaitForFences(device, 1, &fence, true, UINT64_MAX));
        vkResetFences(device, 1, &fence);

        uint32_t semaphoreIndex = renderer.getSemaphoreIndex();
        VkSemaphore aovsReadySemaphore = frameBuffersReadySemaphores[semaphoreIndex];
        VkSemaphore processingFinishedSemaphore = frameBuffersReleaseSemaphores[semaphoreIndex];

        postProcessing.run(aovsReadySemaphore, processingFinishedSemaphore);
        vkQueueSubmit(queue, 0, nullptr, fence);

        if (i == 0 || i == ITERATIONS - 1) 
        {
            postProcessing.waitQueueIdle();
            size_t size;
            postProcessing.getOutput(nullptr, 0, &size);
            output.resize(size);
            postProcessing.getOutput(output.data(), size, nullptr);

            auto resultPath = exeDirPath / ("result_with_interop_" + std::to_string(i) + ".png");
            std::filesystem::remove(resultPath);
            savePngImage(resultPath.string().c_str(), output.data(), WIDTH, HEIGHT, format);
        }

        currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
    }

    postProcessing.waitQueueIdle();

    for (auto f : fences) {
        vkDestroyFence(device, f, nullptr);
    }

    for (auto s : frameBuffersReleaseSemaphores) {
        vkDestroySemaphore(device, s, nullptr);
    }
}


void runWithoutInterop(const std::filesystem::path& exeDirPath, int deviceId)
{
     std::filesystem::path hybridproDll = exeDirPath / "HybridPro.dll";
    std::filesystem::path hybridproCacheDir = exeDirPath / "hybridpro_cache";
    std::filesystem::path assetsDir = exeDirPath;

    RprPpImageFormat format = RPRPP_IMAGE_FROMAT_R32G32B32A32_SFLOAT;
    RprPostProcessing postProcessing(deviceId);
    postProcessing.resize(WIDTH, HEIGHT, format);

    HybridProRenderer renderer(deviceId, std::nullopt, hybridproDll, hybridproCacheDir, assetsDir);
    renderer.resize(WIDTH, HEIGHT);

    std::vector<uint8_t> output;
    for (size_t i = 0; i < ITERATIONS; i++) {
        renderer.render();
        
        copyRprFbToPpStagingBuffer(renderer, postProcessing, RPR_AOV_COLOR);
        postProcessing.copyStagingBufferToAovColor();
        copyRprFbToPpStagingBuffer(renderer, postProcessing, RPR_AOV_OPACITY);
        postProcessing.copyStagingBufferToAovOpacity();
        copyRprFbToPpStagingBuffer(renderer, postProcessing, RPR_AOV_SHADOW_CATCHER);
        postProcessing.copyStagingBufferToAovShadowCatcher();
        copyRprFbToPpStagingBuffer(renderer, postProcessing, RPR_AOV_REFLECTION_CATCHER);
        postProcessing.copyStagingBufferToAovReflectionCatcher();
        copyRprFbToPpStagingBuffer(renderer, postProcessing, RPR_AOV_MATTE_PASS);
        postProcessing.copyStagingBufferToAovMattePass();
        copyRprFbToPpStagingBuffer(renderer, postProcessing, RPR_AOV_BACKGROUND);
        postProcessing.copyStagingBufferToAovBackground();

        postProcessing.run();
        postProcessing.waitQueueIdle();

        if (i == 0 || i == ITERATIONS - 1) 
        {
            size_t size;
            postProcessing.getOutput(nullptr, 0, &size);
            output.resize(size);
            postProcessing.getOutput(output.data(), size, nullptr);

            auto resultPath = exeDirPath / ("result_without_interop_" + std::to_string(i) + ".png");
            std::filesystem::remove(resultPath);
            savePngImage(resultPath.string().c_str(), output.data(), WIDTH, HEIGHT, format);
        }
    }
}

void copyRprFbToPpStagingBuffer(HybridProRenderer& r, RprPostProcessing& pp, rpr_aov aov)
{
    size_t size;
    r.getAov(aov, nullptr, 0u, &size);
    void* data = pp.mapStagingBuffer(size);
    r.getAov(aov, data, size, nullptr);
    pp.unmapStagingBuffer();
}

inline uint8_t floatToByte(float value)
{
    if (value >= 1.0f) {
        return 255;
    }
    if (value <= 0.0f) {
        return 0;
    }
    return roundf(value * 255.0f);
}

void savePngImage(const char* filename, uint8_t* img, uint32_t width, uint32_t height, RprPpImageFormat format)
{
    size_t numComponents = 4;
    uint8_t* dst = img;
    std::vector<uint8_t> vDst;
    if (format != RPRPP_IMAGE_FROMAT_R8G8B8A8_UNORM) {
        vDst.resize(width * height * numComponents);
        dst = vDst.data();
        switch (format) {
        case RPRPP_IMAGE_FROMAT_R32G32B32A32_SFLOAT: {
            float* hdrImg = (float*)img;
            for (size_t i = 0; i < width * height * numComponents; i++) {
                dst[i] = floatToByte(hdrImg[i]);
            }
            /* code */
            break;
        }
        default:
            throw std::runtime_error("not implemented image format conversation");
        }
    }

    stbi_write_png(filename, width, height, numComponents, dst, width * numComponents);
}