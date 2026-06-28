#include "window.hpp"

#include <stb/stb_image.h>

#include <stdexcept>

#ifndef NDEBUG
#include "core/logger.hpp"
#endif

namespace win {

Window::Window(int width, int height, const std::string& title) : m_fbWidth(width), m_fbHeight(height) {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW: Failed to initialize");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
        glfwWindowHintString(GLFW_WAYLAND_APP_ID, "nedizblox_client");
    }

    #ifndef NDEBUG
    switch (glfwGetPlatform()) {
        case GLFW_PLATFORM_WIN32:
            core::logger::info("Using WSI platform Win32", true);
            break;
        case GLFW_PLATFORM_WAYLAND:
            core::logger::info("Using WSI platform Wayland", true);
            break;
        case GLFW_PLATFORM_X11:
            core::logger::info("Using WSI platform X11", true);
            break;
        case GLFW_PLATFORM_COCOA:
            core::logger::info("Using WSI platform Cocoa", true);
            break;
        case GLFW_PLATFORM_NULL:
            core::logger::info("Using WSI platform Null", true);
            break;
        default:
            core::logger::warn("Failed to fetch WSI platform", true);
            break;
    }
    #endif

    m_window = glfwCreateWindow(m_fbWidth, m_fbHeight, title.c_str(), nullptr, nullptr);

    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("GLFW: Failed to create window");
    }

    // center window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (mode) {
        int xpos = (mode->width - m_fbWidth) / 2;
        int ypos = (mode->height - m_fbHeight) / 2;

        glfwSetWindowPos(m_window, xpos, ypos);
    }

    glfwSetWindowUserPointer(m_window, this);

    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetCharCallback(m_window, charCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, mousePosCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetWindowSizeCallback(m_window, winResizeCallback);
    glfwSetFramebufferSizeCallback(m_window, fbResizeCallback);
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::setTitle(const std::string& title) { glfwSetWindowTitle(m_window, title.c_str()); }

void Window::setIcon(const std::string& filePath) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        throw std::runtime_error("STB: Failed to open texture image file: " + filePath);
    }

    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = pixels;

    glfwSetWindowIcon(m_window, 1, &image);

    stbi_image_free(pixels);
}

void Window::update() {
    m_mouseDelta = glm::vec2(0.0f);
    m_scrollDelta = glm::vec2(0.0f);
    m_keysPrev = m_keys;

    m_inputCodepoints.clear();

    float currentFrame = static_cast<float>(glfwGetTime());
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;

    glfwPollEvents();

    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    m_minimized = (width == 0 || height == 0);
}

void Window::createSurface(VkInstance instance, VkSurfaceKHR* surface) {
    if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create window surface");
    }
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        self->m_keys[key] = true;
    } else if (action == GLFW_RELEASE) {
        self->m_keys[key] = false;
    }
}

void Window::charCallback(GLFWwindow* window, unsigned int codepoint) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    self->m_inputCodepoints.push_back(codepoint);
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        self->m_mouseButtons[button] = true;
    } else if (action == GLFW_RELEASE) {
        self->m_mouseButtons[button] = false;
    }
}

void Window::mousePosCallback(GLFWwindow* window, double x, double y) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (self->m_firstMouse) {
        self->m_mousePos = glm::vec2(x, y);
        self->m_firstMouse = false;
        return;
    }

    self->m_mouseDelta = glm::vec2(x - self->m_mousePos.x, self->m_mousePos.y - y);

    self->m_mousePos = glm::vec2(x, y);
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    self->m_scrollDelta = glm::vec2(xoffset, yoffset);
}

void Window::winResizeCallback(GLFWwindow* window, int width, int height) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    self->m_winWidth = width;
    self->m_winHeight = height;
}

void Window::fbResizeCallback(GLFWwindow* window, int width, int height) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    self->m_fbWidth = width;
    self->m_fbHeight = height;

    self->m_resized = true;
}

} // namespace win