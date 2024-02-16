#pragma once

#include "capi/rprpp.h"
#include "vk.h"
#include <optional>

namespace vk::helper {

enum class DeviceInfo {
    eName = RPRPP_DEVICE_INFO_NAME,
};

struct DeviceContext {
    vk::raii::Context context;
    vk::raii::Instance instance;
    std::optional<vk::raii::DebugUtilsMessengerEXT> debugUtilMessenger;
    vk::raii::PhysicalDevice physicalDevice;
    vk::raii::Device device;
    vk::raii::Queue queue;
    uint32_t queueFamilyIndex;
};

struct Buffer {
    vk::raii::Buffer buffer;
    vk::raii::DeviceMemory memory;
};

struct Image {
    vk::raii::Image image;
    vk::raii::DeviceMemory memory;
    vk::raii::ImageView view;
    uint32_t width;
    uint32_t height;
    vk::AccessFlags access;
    vk::ImageLayout layout;
    vk::PipelineStageFlags stage;
};

void getDeviceInfo(uint32_t deviceId, DeviceInfo info, void* data, size_t size, size_t* sizeRet);
uint32_t getDeviceCount();

DeviceContext createDeviceContext(bool enableValidationLayers, uint32_t deviceId);

Image createImage(const DeviceContext& dctx,
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageUsageFlags usage,
    HANDLE outputDx11TextureHandle = nullptr);

Buffer createBuffer(const DeviceContext& dctx,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties);

}