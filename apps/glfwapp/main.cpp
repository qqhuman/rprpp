#include "NoAovsInteropApp.h"
#include "WithAovsInteropApp.h"
#include <iostream>

#define WIDTH 2000
#define HEIGHT 1000
#define INTEROP true
#define DEVICE_ID 0
// please note that when we use frames in flight > 1 
// hybridpro produces Validation Error with VK_OBJECT_TYPE_QUERY_POOL message looks like "query not reset. After query pool creation"
#define FRAMES_IN_FLIGHT 1


int main(int argc, const char* argv[])
{
    GpuIndices gpus = { .dx11 = DEVICE_ID, .vk = DEVICE_ID };
    std::cout << "GlfwApp started..." << std::endl;
    std::filesystem::path exeDirPath = std::filesystem::path(argv[0]).parent_path();
    Paths paths = {
        .hybridproDll = exeDirPath / "HybridPro.dll",
        .hybridproCacheDir = exeDirPath / "hybridpro_cache",
        .assetsDir = exeDirPath,
    };

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
#if INTEROP
    if (FRAMES_IN_FLIGHT > 1) {
        std::cerr << "doesn't support FRAMES_IN_FLIGHT > 1 right now" << std::endl;
        throw std::runtime_error("doesn't support FRAMES_IN_FLIGHT > 1 right now");
    }
    WithAovsInteropApp app(WIDTH, HEIGHT, FRAMES_IN_FLIGHT, paths, gpus);
#else
    NoAovsInteropApp app(WIDTH, HEIGHT, paths, gpus);
#endif
    try {
        app.run();
    } catch (const std::runtime_error& e) {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }
    std::cout << "GlfwApp finished..." << std::endl;
    return 0;
}