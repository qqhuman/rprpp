#include "HybridProRenderer.h"
#include "rpr_helper.h"
#include <RadeonProRender_VK.h>
#include <set>

HybridProRenderer::HybridProRenderer(int deviceId,
    const std::optional<HybridProInteropInfo>& interopInfo,
    const std::filesystem::path& hybridproDll,
    const std::filesystem::path& hybridproCacheDir,
    const std::filesystem::path& assetsDir)
    : m_interopInfo(interopInfo)
{
    std::cout << "HybridProRenderer()" << std::endl;

    std::vector<rpr_context_properties> properties;
    rpr_creation_flags creation_flags = intToRprCreationFlag(deviceId);

    VkInteropInfo::VkInstance instance_;
    VkInteropInfo interopInfo_;

    if (m_interopInfo.has_value()) {
        instance_.physical_device = m_interopInfo.value().physicalDevice;
        instance_.device = m_interopInfo.value().device;

        interopInfo_.instance_count = 1,
        interopInfo_.instances = &instance_;
        interopInfo_.main_instance_index = 0,
        interopInfo_.frames_in_flight = m_interopInfo.value().framesInFlight,
        interopInfo_.framebuffers_release_semaphores = m_interopInfo.value().frameBuffersReleaseSemaphores;
        properties.push_back((rpr_context_properties)RPR_CONTEXT_CREATEPROP_VK_INTEROP_INFO);
        properties.push_back((rpr_context_properties)&interopInfo_);
        creation_flags = RPR_CREATION_FLAGS_ENABLE_VK_INTEROP;
    }

    properties.push_back(0);

    rpr_int pluginId = rprRegisterPlugin(hybridproDll.string().c_str());
    rpr_int plugins[] = { pluginId };
    RPR_CHECK(rprCreateContext(
        RPR_API_VERSION,
        plugins,
        sizeof(plugins) / sizeof(plugins[0]),
        creation_flags,
        properties.data(),
        hybridproCacheDir.string().c_str(),
        &m_context));
    RPR_CHECK(rprContextSetActivePlugin(m_context, plugins[0]));
    RPR_CHECK(rprContextSetParameterByKey1u(m_context, RPR_CONTEXT_Y_FLIP, RPR_TRUE));
    RPR_CHECK(rprContextSetParameterByKey1u(m_context, RPR_CONTEXT_IBL_DISPLAY, RPR_FALSE));
    RPR_CHECK(rprContextSetParameterByKey1u(m_context, RPR_CONTEXT_MAX_RECURSION, 10));
    RPR_CHECK(rprContextSetParameterByKey1f(m_context, RPR_CONTEXT_DISPLAY_GAMMA, 1.0f));
    RPR_CHECK(rprContextSetParameterByKey1u(m_context, RPR_CONTEXT_ITERATIONS, 1));

    if (m_interopInfo.has_value()) {
        m_frameBuffersReadySemaphores.resize(m_interopInfo.value().framesInFlight);
        RPR_CHECK(rprContextGetInfo(m_context,
            (rpr_context_info)RPR_CONTEXT_FRAMEBUFFERS_READY_SEMAPHORES,
            sizeof(VkSemaphore) * m_interopInfo.value().framesInFlight,
            m_frameBuffersReadySemaphores.data(),
            nullptr));
    }

    RPR_CHECK(rprContextGetFunctionPtr(m_context,
        RPR_CONTEXT_FLUSH_FRAMEBUFFERS_FUNC_NAME,
        (void**)(&rprContextFlushFrameBuffers)));

    RPR_CHECK(rprContextCreateScene(m_context, &m_scene));
    RPR_CHECK(rprContextSetScene(m_context, m_scene));
    RPR_CHECK(rprContextCreateMaterialSystem(m_context, 0, &m_matsys));

    // Camera
    {
        RPR_CHECK(rprContextCreateCamera(m_context, &m_camera));
        RPR_CHECK(rprSceneSetCamera(m_scene, m_camera));
        RPR_CHECK(rprObjectSetName(m_camera, (rpr_char*)"camera"));
        RPR_CHECK(rprCameraSetMode(m_camera, RPR_CAMERA_MODE_PERSPECTIVE));
        RPR_CHECK(rprCameraLookAt(m_camera,
            8.0f, 2.0f, 8.0f, // pos
            0.0f, 0.0f, 0.0f, // at
            0.0f, 1.0f, 0.0f // up
            ));
    }

    // teapot material
    {
        RPR_CHECK(rprMaterialSystemCreateNode(m_matsys, RPR_MATERIAL_NODE_UBERV2, &m_teapotMaterial));
        RPR_CHECK(rprMaterialNodeSetInputFByKey(m_teapotMaterial, RPR_MATERIAL_INPUT_UBER_DIFFUSE_COLOR, 0.7f, 0.2f, 0.0f, 1.0f));
        RPR_CHECK(rprMaterialNodeSetInputFByKey(m_teapotMaterial, RPR_MATERIAL_INPUT_UBER_DIFFUSE_WEIGHT, 1.0f, 1.0f, 1.0f, 1.0f));
    }

    // teapot
    {
        auto teapotShapePath = assetsDir / "teapot.obj";
        m_teapot = ImportOBJ(teapotShapePath.string(), m_context);
        RPR_CHECK(rprSceneAttachShape(m_scene, m_teapot));
        RPR_CHECK(rprShapeSetMaterial(m_teapot, m_teapotMaterial));
        rpr_float transform[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.3f, 0.0f, 1.0f,
        };
        RPR_CHECK(rprShapeSetTransform(m_teapot, false, transform));
    }

    // shadow reflection catcher material
    {
        RPR_CHECK(rprMaterialSystemCreateNode(m_matsys, RPR_MATERIAL_NODE_UBERV2, &m_shadowReflectionCatcherMaterial));
        RPR_CHECK(rprMaterialNodeSetInputFByKey(m_shadowReflectionCatcherMaterial, RPR_MATERIAL_INPUT_UBER_DIFFUSE_COLOR, 0.3f, 0.3f, 0.3f, 1.0f));
        RPR_CHECK(rprMaterialNodeSetInputFByKey(m_shadowReflectionCatcherMaterial, RPR_MATERIAL_INPUT_UBER_DIFFUSE_WEIGHT, 1.0f, 1.0f, 1.0f, 1.0f));

        //RPR_CHECK(rprMaterialNodeSetInputFByKey(m_shadowReflectionCatcherMaterial, RPR_MATERIAL_INPUT_UBER_REFLECTION_COLOR, 0.3f, 0.3f, 0.3f, 1.0f));
        RPR_CHECK(rprMaterialNodeSetInputFByKey(m_shadowReflectionCatcherMaterial, RPR_MATERIAL_INPUT_UBER_REFLECTION_WEIGHT, 1.0f, 1.0f, 1.0f, 1.0f));
        RPR_CHECK(rprMaterialNodeSetInputFByKey(m_shadowReflectionCatcherMaterial, RPR_MATERIAL_INPUT_UBER_REFLECTION_IOR, 50.0f, 50.0f, 50.0f, 50.0f));
        RPR_CHECK(rprMaterialNodeSetInputFByKey(m_shadowReflectionCatcherMaterial, RPR_MATERIAL_INPUT_UBER_REFLECTION_ROUGHNESS, 0.0f, 0.0f, 0.0f, 0.0f));
    }

    // shadow reflection catcher plane
    {
        std::vector<rpr_float> vertices = {
            -1.0f, 0.0f, -1.0f,
            -1.0f, 0.0f,  1.0f,
            1.0f, 0.0f,  1.0f,
            1.0f, 0.0f, -1.0f,
        };

        std::vector<rpr_float> normals = {
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
        };

        std::vector<rpr_float> uvs = {
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
        };

        std::vector<rpr_int> indices = {
            3,1,0,
            2,1,3
        };

        std::vector<rpr_int> numFaceVertices;
        numFaceVertices.resize(indices.size() / 3, 3);

        RPR_CHECK(rprContextCreateMesh(m_context,
            (rpr_float const*)&vertices[0], vertices.size() / 3, 3 * sizeof(rpr_float),
            (rpr_float const*)&normals[0], normals.size() / 3, 3 * sizeof(rpr_float),
            (rpr_float const*)&uvs[0], uvs.size() / 2, 2 * sizeof(rpr_float),
            (rpr_int const*)&indices[0], sizeof(rpr_int),
            (rpr_int const*)&indices[0], sizeof(rpr_int),
            (rpr_int const*)&indices[0], sizeof(rpr_int),
            (rpr_int const*)&numFaceVertices[0], numFaceVertices.size(), &m_shadowReflectionCatcherPlane));
        RPR_CHECK(rprSceneAttachShape(m_scene, m_shadowReflectionCatcherPlane));
        RPR_CHECK(rprShapeSetMaterial(m_shadowReflectionCatcherPlane, m_shadowReflectionCatcherMaterial));
        rpr_float transform[16] = {
            8.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 8.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 8.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        RPR_CHECK(rprShapeSetTransform(m_shadowReflectionCatcherPlane, false, transform));
        RPR_CHECK(rprShapeSetShadowCatcher(m_shadowReflectionCatcherPlane, true));
        RPR_CHECK(rprShapeSetReflectionCatcher(m_shadowReflectionCatcherPlane, true));
    }

    // ibl image
    {
        auto iblPath = assetsDir / "envLightImage.exr";
        RPR_CHECK(rprContextCreateImageFromFile(m_context, iblPath.string().c_str(), &m_iblimage));
        RPR_CHECK(rprObjectSetName(m_iblimage, "iblimage"));
    }

    // env light
    {
        RPR_CHECK(rprContextCreateEnvironmentLight(m_context, &m_light));
        RPR_CHECK(rprObjectSetName(m_light, "light"));
        RPR_CHECK(rprEnvironmentLightSetImage(m_light, m_iblimage));
        rpr_float float_P16_4[] = {
            -1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        RPR_CHECK(rprLightSetTransform(m_light, false, (rpr_float*)&float_P16_4));
        RPR_CHECK(rprSceneSetEnvironmentLight(m_scene, m_light));
    }
}

HybridProRenderer::~HybridProRenderer()
{
    std::cout << "~HybridProRenderer()" << std::endl;
    RPR_CHECK(rprSceneDetachLight(m_scene, m_light));
    RPR_CHECK(rprSceneDetachShape(m_scene, m_teapot));
    for (auto& aov : m_aovs) {
        RPR_CHECK(rprObjectDelete(aov.second));
    }
    RPR_CHECK(rprObjectDelete(m_light));
    m_light = nullptr;
    RPR_CHECK(rprObjectDelete(m_iblimage));
    m_iblimage = nullptr;
    RPR_CHECK(rprObjectDelete(m_teapot));
    m_teapot = nullptr;
    RPR_CHECK(rprObjectDelete(m_shadowReflectionCatcherPlane));
    m_shadowReflectionCatcherPlane = nullptr;
    RPR_CHECK(rprObjectDelete(m_camera));
    m_camera = nullptr;
    RPR_CHECK(rprObjectDelete(m_scene));
    m_scene = nullptr;
    RPR_CHECK(rprObjectDelete(m_shadowReflectionCatcherMaterial));
    m_shadowReflectionCatcherMaterial = nullptr;
    RPR_CHECK(rprObjectDelete(m_teapotMaterial));
    m_teapotMaterial = nullptr;
    RPR_CHECK(rprObjectDelete(m_matsys));
    m_matsys = nullptr;
    CheckNoLeak(m_context);
    RPR_CHECK(rprObjectDelete(m_context));
    m_context = nullptr;
}

const std::vector<uint8_t>& HybridProRenderer::readAovBuff(rpr_aov aovKey)
{
    rpr_framebuffer aov = m_aovs[aovKey];
    size_t size;
    RPR_CHECK(rprFrameBufferGetInfo(aov, RPR_FRAMEBUFFER_DATA, 0, nullptr, &size));

    if (m_tmpAovBuff.size() != size) {
        m_tmpAovBuff.resize(size);
    }

    uint8_t* buff = nullptr;
    RPR_CHECK(rprFrameBufferGetInfo(aov, RPR_FRAMEBUFFER_DATA, size, m_tmpAovBuff.data(), nullptr));
    return m_tmpAovBuff;
}

void HybridProRenderer::resize(uint32_t width, uint32_t height)
{
    if (m_width != width || m_height != height) {
        for (auto& aov : m_aovs) {
            RPR_CHECK(rprObjectDelete(aov.second));
        }

        std::set<rpr_aov> aovs = {
            RPR_AOV_COLOR,
            RPR_AOV_OPACITY,
            RPR_AOV_SHADOW_CATCHER,
            RPR_AOV_REFLECTION_CATCHER,
            RPR_AOV_MATTE_PASS,
            RPR_AOV_BACKGROUND,
        };

        const rpr_framebuffer_desc desc = { width, height };
        const rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
        for (auto aov : aovs) {
            rpr_framebuffer fb;
            RPR_CHECK(rprContextCreateFrameBuffer(m_context, fmt, &desc, &fb));
            RPR_CHECK(rprContextSetAOV(m_context, aov, fb));
            m_aovs[aov] = fb;
        }

        const float sensorHeight = 24.0f;
        float aspectRatio = (float)width / height;
        RPR_CHECK(rprCameraSetSensorSize(m_camera, sensorHeight * aspectRatio, sensorHeight));

        m_width = width;
        m_height = height;
    }
}

void HybridProRenderer::render()
{
    RPR_CHECK(rprContextRender(m_context));
}

std::vector<VkSemaphore> HybridProRenderer::getFrameBuffersReadySemaphores()
{
    if (!m_interopInfo.has_value()) {
        throw std::runtime_error("getFrameBuffersReadySemaphores is unavailable without vulkan interop");
    }

    return m_frameBuffersReadySemaphores;
}

uint32_t HybridProRenderer::getSemaphoreIndex()
{
    if (!m_interopInfo.has_value()) {
        throw std::runtime_error("getSemaphoreIndex is unavailable without vulkan interop");
    }

    std::uint32_t semaphoreIndex = 0;
    RPR_CHECK(rprContextGetInfo(m_context,
        (rpr_context_info)RPR_CONTEXT_INTEROP_SEMAPHORE_INDEX,
        sizeof(semaphoreIndex),
        &semaphoreIndex,
        nullptr));

    return semaphoreIndex;
}

void HybridProRenderer::flushFrameBuffers()
{
    if (!m_interopInfo.has_value()) {
        throw std::runtime_error("flushFrameBuffers is unavailable without vulkan interop");
    }

    RPR_CHECK(rprContextFlushFrameBuffers(m_context));
}

VkImage HybridProRenderer::getAovVkImage(rpr_aov aov)
{
    if (!m_interopInfo.has_value()) {
        throw std::runtime_error("getAovVkImage is unavailable without vulkan interop");
    }

    VkImage image;
    RPR_CHECK(rprFrameBufferGetInfo(getAov(aov), (rpr_framebuffer_info)RPR_VK_IMAGE_OBJECT, sizeof(VkImage), &image, 0));
    return image;
}

float HybridProRenderer::getFocalLength()
{
    float focalLength;
    RPR_CHECK(rprCameraGetInfo(m_camera, RPR_CAMERA_FOCAL_LENGTH, sizeof(focalLength), &focalLength, nullptr));
    return focalLength;
}

void HybridProRenderer::saveResultTo(const std::filesystem::path& path, rpr_aov aov)
{
    RPR_CHECK(rprFrameBufferSaveToFile(m_aovs[aov], path.string().c_str()));
}


void HybridProRenderer::getAov(rpr_aov aov, void* data, size_t size, size_t* retSize) const
{
    rpr_framebuffer rprfb = m_aovs.at(aov);
    if (retSize != nullptr) {
        RPR_CHECK(rprFrameBufferGetInfo(rprfb, RPR_FRAMEBUFFER_DATA, 0, nullptr, retSize));
    }

    if (data != nullptr && size > 0) {
        RPR_CHECK(rprFrameBufferGetInfo(rprfb, RPR_FRAMEBUFFER_DATA, size, data, nullptr));
    }
}