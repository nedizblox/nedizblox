#include "window.hpp"

#include <stb/stb_image.h>

#include <stdexcept>

namespace win {

Window::Window(int width, int height, const std::string& title) : m_width(width), m_height(height) {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW: Failed to initialize");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
        glfwWindowHintString(GLFW_WAYLAND_APP_ID, "nedizblox_client");
    }

    m_window = glfwCreateWindow(m_width, m_height, title.c_str(), nullptr, nullptr);

    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("GLFW: Failed to create window");
    }

    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // center window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (mode) {
        int xpos = (mode->width - m_width) / 2;
        int ypos = (mode->height - m_height) / 2;

        glfwSetWindowPos(m_window, xpos, ypos);
    }

    glfwSetWindowUserPointer(m_window, this);

    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetCharCallback(m_window, charCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, mousePosCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetFramebufferSizeCallback(m_window, resizeCallback);
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
    m_mouseRel = glm::vec2(0.0f);

    m_keysPrev = m_keys;

    float currentFrame = static_cast<float>(glfwGetTime());
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;

    glfwPollEvents();

    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    m_minimized = (width == 0 || height == 0);
}

VkExtent2D Window::getExtent() const {
    VkExtent2D extent{};
    extent.width = static_cast<uint32_t>(m_width);
    extent.height = static_cast<uint32_t>(m_height);

    return extent;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        self->m_keys[key] = true;
    } else if (action == GLFW_RELEASE) {
        self->m_keys[key] = false;
    }
}

void Window::charCallback(GLFWwindow* window, unsigned int codepoint) {}

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

    self->m_mouseRel = glm::vec2(x - self->m_mousePos.x, self->m_mousePos.y - y);

    self->m_mousePos = glm::vec2(x, y);
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    self->m_scrollDelta = glm::vec2(xoffset, yoffset);
}

void Window::resizeCallback(GLFWwindow* window, int width, int height) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    self->m_width = width;
    self->m_height = height;

    self->m_resized = true;
}

} // namespace win