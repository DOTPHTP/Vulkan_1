#pragma once
#include <vulkan/vulkan.h>
#include <stdexcept>

class SamplerBuilder {
public:
    SamplerBuilder(VkPhysicalDevice physicalDevice)
        : physicalDevice(physicalDevice) {
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
    }

    // ���ø������Թ���
    SamplerBuilder& enableAnisotropy() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        return *this;
    }

    // ����˫���Բ�ֵ����
    SamplerBuilder& setBilinearFiltering() {
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        return *this;
    }

    // ��������ڲ���
    SamplerBuilder& setNearestNeighborFiltering() {
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        return *this;
    }

    // ���ö����
    SamplerBuilder& setMultisampling() {
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;
        return *this;
    }

    // ���õ�ַģʽ
    SamplerBuilder& setAddressMode(VkSamplerAddressMode mode) {
        samplerInfo.addressModeU = mode;
        samplerInfo.addressModeV = mode;
        samplerInfo.addressModeW = mode;
        return *this;
    }

    // ����������
    VkSampler build(VkDevice device) {
        VkSampler sampler;
        if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
        return sampler;
    }

private:
    VkPhysicalDevice physicalDevice;
    VkSamplerCreateInfo samplerInfo{};
};
