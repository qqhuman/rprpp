#pragma once

#include "Buffer.h"

namespace rprpp {

class Context;

template <class T>
class UniformObjectBuffer {
public:
    explicit UniformObjectBuffer(Context* context);

    T& data() noexcept;
    const T& data() const noexcept;

    [[nodiscard]] size_t size() const noexcept;

    [[nodiscard]] vk::Buffer buffer() const noexcept;

    [[nodiscard]] bool dirty() const noexcept;
    void markDirty() noexcept;

    void update();

    UniformObjectBuffer(const Buffer&) = delete;
    UniformObjectBuffer& operator=(const Buffer&) = delete;

private:
    Buffer createBuffer(Context* context);

    bool m_dirty;
    T m_data;
    Buffer m_buffer;
};

template <class T>
Buffer UniformObjectBuffer<T>::createBuffer(Context* context)
{
    return Buffer(context,
        sizeof(T),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

template <class T>
UniformObjectBuffer<T>::UniformObjectBuffer(Context* context)
    : m_buffer(createBuffer(context))
    , m_dirty(true)
{
}

template <class T>
T& UniformObjectBuffer<T>::data() noexcept
{
    return m_data;
}

template <class T>
const T& UniformObjectBuffer<T>::data() const noexcept
{
    return m_data;
}

template <class T>
bool UniformObjectBuffer<T>::dirty() const noexcept
{
    return m_dirty;
}

template <class T>
size_t UniformObjectBuffer<T>::size() const noexcept
{
    return m_buffer.size();
}

template <class T>
vk::Buffer UniformObjectBuffer<T>::buffer() const noexcept
{
    return m_buffer.get();
}

template <class T>
void UniformObjectBuffer<T>::update()
{
    void* data = m_buffer.map(sizeof(T));
    std::memcpy(data, &m_data, sizeof(T));
    m_buffer.unmap();
    m_dirty = false;
}

template <class T>
void UniformObjectBuffer<T>::markDirty() noexcept
{
    m_dirty = true;
}

}