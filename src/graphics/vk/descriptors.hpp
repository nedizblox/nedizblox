#pragma once

#include "device.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace gfx::vk {

class DescriptorSetLayout {
public:
    class Builder {
    public:
        Builder(Device& device) : m_device(device) {}

        Builder&
        addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);

        std::unique_ptr<DescriptorSetLayout> build() const;

    private:
        Device& m_device;

        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings{};
    };

    DescriptorSetLayout(Device& device, const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings);
    ~DescriptorSetLayout();

    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    Device& m_device;

    VkDescriptorSetLayout m_descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

    friend class DescriptorWriter;
};

class DescriptorPool {
public:
    class Builder {
    public:
        Builder(Device& device) : m_device(device) {}

        Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);

        Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder& setMaxSets(uint32_t count);

        std::unique_ptr<DescriptorPool> build() const;

    private:
        Device& m_device;

        std::vector<VkDescriptorPoolSize> m_poolSizes{};
        VkDescriptorPoolCreateFlags m_poolFlags = 0;

        uint32_t m_maxSets = 1000;
    };

    DescriptorPool(
        Device& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes);
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }

    bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
    void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

    void resetPool();

private:
    Device& m_device;

    VkDescriptorPool m_descriptorPool;

    friend class DescriptorWriter;
};

class DescriptorWriter {
public:
    DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool) :
        m_setLayout(setLayout), m_pool(pool) {}

    DescriptorWriter(const DescriptorWriter&) = delete;
    DescriptorWriter& operator=(const DescriptorWriter&) = delete;

    DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
    DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);
    DescriptorWriter& writeImageArray(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfos);

    bool build(VkDescriptorSet& set);

    void overwrite(VkDescriptorSet& set);

private:
    DescriptorSetLayout& m_setLayout;
    DescriptorPool& m_pool;

    std::vector<VkWriteDescriptorSet> m_writes;
};

} // namespace gfx::vk