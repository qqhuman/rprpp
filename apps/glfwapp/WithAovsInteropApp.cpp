#include "WithAovsInteropApp.h"

#define FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

inline RprPpImageFormat to_rprppformat(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return RPRPP_IMAGE_FROMAT_R8G8B8A8_UNORM;
    default:
        throw std::runtime_error("unsupported image format");
    }
}

WithAovsInteropApp::WithAovsInteropApp(int width, int height, uint32_t framesInFlight, Paths paths, GpuIndices gpuIndices)
    : m_width(width)
    , m_height(height)
    , m_framesInFlight(framesInFlight)
    , m_paths(paths)
    , m_gpuIndices(gpuIndices)
{
    std::cout << "WithAovsInteropApp()" << std::endl;
}

WithAovsInteropApp::~WithAovsInteropApp()
{
    std::cout << "~WithAovsInteropApp()" << std::endl;
    for (auto f : m_fences) {
        vkDestroyFence(m_postProcessing->getVkDevice(), f, nullptr);
    }

    for (auto s : m_frameBuffersReleaseSemaphores) {
        vkDestroySemaphore(m_postProcessing->getVkDevice(), s, nullptr);
    }

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void WithAovsInteropApp::run()
{
    initWindow();
    findAdapter();
    intiSwapChain();
    initHybridProAndPostProcessing();
    resize(m_width, m_height);
    mainLoop();
}

void WithAovsInteropApp::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(m_width, m_height, "VkDx11 Interop", nullptr, nullptr);
    m_hWnd = glfwGetWin32Window(m_window);
    glfwSetFramebufferSizeCallback(m_window, WithAovsInteropApp::onResize);
    glfwSetWindowUserPointer(m_window, this);
}

void WithAovsInteropApp::findAdapter()
{
    ComPtr<IDXGIFactory4> factory;
    DX_CHECK(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIFactory6> factory6;
    DX_CHECK(factory->QueryInterface(IID_PPV_ARGS(&factory6)));

    std::cout << "All DXGI Adapters:" << std::endl;

    ComPtr<IDXGIAdapter1> tmpAdapter;
    DXGI_ADAPTER_DESC selectedAdapterDesc;
    int adapterCount = 0;
    for (UINT adapterIndex = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&tmpAdapter))); ++adapterIndex) {
        adapterCount++;

        DXGI_ADAPTER_DESC desc;
        tmpAdapter->GetDesc(&desc);

        std::wcout << "\t" << adapterIndex << ". " << desc.Description << std::endl;

        if (adapterIndex == m_gpuIndices.dx11) {
            m_adapter = tmpAdapter;
            selectedAdapterDesc = desc;
        }
    }

    if (adapterCount <= m_gpuIndices.dx11) {
        throw std::runtime_error("could not find a IDXGIAdapter1, gpuIndices.dx11 is out of range");
    }

    std::wcout << "Selected adapter: " << selectedAdapterDesc.Description << std::endl;
}

void WithAovsInteropApp::intiSwapChain()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
    scd.BufferDesc.Width = 0; // use window width
    scd.BufferDesc.Height = 0; // use window height
    scd.BufferDesc.Format = FORMAT;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 1;
    scd.OutputWindow = m_hWnd;
    scd.Windowed = TRUE;
    auto featureLevel = D3D_FEATURE_LEVEL_11_1;
    DX_CHECK(D3D11CreateDeviceAndSwapChain(
        m_adapter.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        0,
        &featureLevel,
        1,
        D3D11_SDK_VERSION,
        &scd,
        &m_swapChain,
        &m_device,
        nullptr,
        &m_deviceContex));
}

void WithAovsInteropApp::initHybridProAndPostProcessing()
{
    m_postProcessing = std::make_unique<RprPostProcessing>(m_gpuIndices.vk);
    m_postProcessing->setFramesInFlihgt(m_framesInFlight);

    for (uint32_t i = 0; i < m_framesInFlight; i++) {
        VkSemaphore semaphore;
        VkSemaphoreCreateInfo semaphoreInfo { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(m_postProcessing->getVkDevice(), &semaphoreInfo, nullptr, &semaphore));
        m_frameBuffersReleaseSemaphores.push_back(semaphore);

        VkFence fence;
        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(m_postProcessing->getVkDevice(), &fenceInfo, nullptr, &fence));
        m_fences.push_back(fence);
    }

    // set frame buffers realese to signal state
    {
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pSignalSemaphores = m_frameBuffersReleaseSemaphores.data();
        submitInfo.signalSemaphoreCount = m_frameBuffersReleaseSemaphores.size();
        VK_CHECK(vkQueueSubmit(m_postProcessing->getVkQueue(), 1, &submitInfo, nullptr));
    }

    HybridProInteropInfo aovsInteropInfo = HybridProInteropInfo {
        .physicalDevice = m_postProcessing->getVkPhysicalDevice(),
        .device = m_postProcessing->getVkDevice(),
        .framesInFlight = m_framesInFlight,
        .frameBuffersReleaseSemaphores = m_frameBuffersReleaseSemaphores.data(),
    };

    m_hybridproRenderer = std::make_unique<HybridProRenderer>(m_gpuIndices.vk, aovsInteropInfo, m_paths.hybridproDll, m_paths.hybridproCacheDir, m_paths.assetsDir);
    m_frameBuffersReadySemaphores = m_hybridproRenderer->getFrameBuffersReadySemaphores();
}

void WithAovsInteropApp::resize(int width, int height)
{
    if (m_width != width || m_height != height || m_backBuffer.Get() == nullptr) {
        m_deviceContex->OMSetRenderTargets(0, nullptr, nullptr);
        m_sharedTextureHandle = nullptr;
        m_sharedTextureResource.Reset();
        m_sharedTexture.Reset();
        m_renderTargetView.Reset();
        m_backBuffer.Reset();
        DX_CHECK(m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

        DX_CHECK(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer)));
        DX_CHECK(m_device->CreateRenderTargetView(m_backBuffer.Get(), nullptr, &m_renderTargetView));
        m_deviceContex->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

        D3D11_TEXTURE2D_DESC sharedTextureDesc = {};
        sharedTextureDesc.Width = width;
        sharedTextureDesc.Height = height;
        sharedTextureDesc.MipLevels = 1;
        sharedTextureDesc.ArraySize = 1;
        sharedTextureDesc.SampleDesc = { 1, 0 };
        sharedTextureDesc.Format = FORMAT;
        sharedTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
        DX_CHECK(m_device->CreateTexture2D(&sharedTextureDesc, nullptr, &m_sharedTexture));
        DX_CHECK(m_sharedTexture->QueryInterface(__uuidof(IDXGIResource1), (void**)&m_sharedTextureResource));
        DX_CHECK(m_sharedTextureResource->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &m_sharedTextureHandle));

        D3D11_VIEWPORT viewport;
        ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

        viewport.Width = width;
        viewport.Height = height;
        m_deviceContex->RSSetViewports(1, &viewport);

        m_hybridproRenderer->resize(width, height);
        RprPpAovsVkInteropInfo aovsVkInteropInfo = {
            .color = (RprPpVkImage)m_hybridproRenderer->getAovVkImage(RPR_AOV_COLOR),
            .opacity = (RprPpVkImage)m_hybridproRenderer->getAovVkImage(RPR_AOV_OPACITY),
            .shadowCatcher = (RprPpVkImage)m_hybridproRenderer->getAovVkImage(RPR_AOV_SHADOW_CATCHER),
            .reflectionCatcher = (RprPpVkImage)m_hybridproRenderer->getAovVkImage(RPR_AOV_REFLECTION_CATCHER),
            .mattePass = (RprPpVkImage)m_hybridproRenderer->getAovVkImage(RPR_AOV_MATTE_PASS),
            .background = (RprPpVkImage)m_hybridproRenderer->getAovVkImage(RPR_AOV_BACKGROUND),
        };
        m_postProcessing->resize(width, height, to_rprppformat(FORMAT), m_sharedTextureHandle, &aovsVkInteropInfo);

        float focalLength = m_hybridproRenderer->getFocalLength() / 1000.0f;
        m_postProcessing->setToneMapFocalLength(focalLength);
        m_width = width;
        m_height = height;
    }
}

void WithAovsInteropApp::onResize(GLFWwindow* window, int width, int height)
{
    auto app = static_cast<WithAovsInteropApp*>(glfwGetWindowUserPointer(window));
    app->resize(width, height);
}

void WithAovsInteropApp::copyRprFbToPpStagingBuffer(rpr_aov aov)
{
    size_t size;
    m_hybridproRenderer->getAov(aov, nullptr, 0u, &size);
    void* data = m_postProcessing->mapStagingBuffer(size);
    m_hybridproRenderer->getAov(aov, data, size, nullptr);
    m_postProcessing->unmapStagingBuffer();
}

void WithAovsInteropApp::mainLoop()
{
    clock_t deltaTime = 0;
    unsigned int frames = 0;
    uint32_t currentFrame = 0;
    while (!glfwWindowShouldClose(m_window)) {
        clock_t beginFrame = clock();
        {
            glfwPollEvents();
            {
                m_hybridproRenderer->render();
                m_hybridproRenderer->flushFrameBuffers();
                
                VkFence fence = m_fences[currentFrame];
                VK_CHECK(vkWaitForFences(m_postProcessing->getVkDevice(), 1, &fence, true, UINT64_MAX));
                vkResetFences(m_postProcessing->getVkDevice(), 1, &fence);

                uint32_t semaphoreIndex = m_hybridproRenderer->getSemaphoreIndex();
                VkSemaphore aovsReadySemaphore = m_frameBuffersReadySemaphores[semaphoreIndex];
                VkSemaphore processingFinishedSemaphore = m_frameBuffersReleaseSemaphores[semaphoreIndex];

                m_postProcessing->run(aovsReadySemaphore, processingFinishedSemaphore);
                vkQueueSubmit(m_postProcessing->getVkQueue(), 0, nullptr, fence);
                currentFrame = (currentFrame + 1) % m_framesInFlight;

                m_postProcessing->waitQueueIdle();
            }


            IDXGIKeyedMutex* km;
            DX_CHECK(m_sharedTextureResource->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&km));
            DX_CHECK(km->AcquireSync(0, INFINITE));
            m_deviceContex->CopyResource(m_backBuffer.Get(), m_sharedTexture.Get());
            DX_CHECK(km->ReleaseSync(0));
            DX_CHECK(m_swapChain->Present(1, 0));
        }
        clock_t endFrame = clock();
        deltaTime += endFrame - beginFrame;
        frames += 1;
        double deltaTimeInSeconds = (deltaTime / (double)CLOCKS_PER_SEC);
        if (deltaTimeInSeconds > 1.0) { // every second
            std::cout << "Iterations per second = "
                      << frames
                      << ", Time per iteration = "
                      << deltaTimeInSeconds * 1000.0 / frames
                      << "ms"
                      << std::endl;
            frames = 0;
            deltaTime -= CLOCKS_PER_SEC;
        }
    }
}