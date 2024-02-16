#pragma once

#include <RadeonProRender.h>
#include <RadeonProRender_Baikal.h>
#include <filesystem>
#include <map>
#include <memory>
#include <vector>
#include <windows.h>
#include <vulkan/vulkan.h>
#include <optional>

struct HybridProInteropInfo {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t framesInFlight;
    VkSemaphore* frameBuffersReleaseSemaphores;
};

class HybridProRenderer {
private:
    std::optional<HybridProInteropInfo> m_interopInfo;
    uint32_t m_width;
    uint32_t m_height;
    std::vector<VkSemaphore> m_frameBuffersReadySemaphores;
    rprContextFlushFrameBuffers_func rprContextFlushFrameBuffers;
    rpr_context m_context = nullptr;
    rpr_material_system m_matsys = nullptr;
    rpr_material_node m_teapotMaterial = nullptr;
    rpr_material_node m_shadowReflectionCatcherMaterial = nullptr;
    rpr_scene m_scene = nullptr;
    rpr_camera m_camera = nullptr;
    rpr_shape m_teapot = nullptr;
    rpr_shape m_shadowReflectionCatcherPlane = nullptr;
    rpr_image m_iblimage = nullptr;
    rpr_light m_light = nullptr;
    std::map<rpr_aov, rpr_framebuffer> m_aovs;
    std::vector<uint8_t> m_tmpAovBuff;

    const std::vector<uint8_t>& readAovBuff(rpr_aov aov);

public:
    HybridProRenderer(int deviceId,
        const std::optional<HybridProInteropInfo>& interopInfo,
        const std::filesystem::path& hybridproDll,
        const std::filesystem::path& hybridproCacheDir,
        const std::filesystem::path& assetsDir);
    HybridProRenderer(const HybridProRenderer&&) = delete;
    HybridProRenderer(const HybridProRenderer&) = delete;
    HybridProRenderer& operator=(const HybridProRenderer&) = delete;
    ~HybridProRenderer();

    void resize(uint32_t width, uint32_t height);
    void render();
    void saveResultTo(const std::filesystem::path& path, rpr_aov aov);
    std::vector<VkSemaphore> getFrameBuffersReadySemaphores();
    uint32_t getSemaphoreIndex();
    void flushFrameBuffers();

    VkImage getAovVkImage(rpr_aov aov);
    void getAov(rpr_aov aov, void* data, size_t size, size_t* retSize) const;
    float getFocalLength();

    inline rpr_framebuffer getAov(rpr_aov aov)
    {
        return m_aovs[aov];
    }
};