#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace gfx::vk::base {

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

#ifndef NDEBUG
const bool ENABLE_VALIDATION_LAYER = true;
#else
const bool ENABLE_VALIDATION_LAYER = false;
#endif

const std::vector<const char*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

} // namespace gfx::vk::base