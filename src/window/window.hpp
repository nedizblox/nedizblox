#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <array>
#include <string>

namespace win {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void update();

    void setTitle(const std::string& title);
    void setIcon(const std::string& filePath);

    void resetResizedState() { m_resized = false; }

    bool isOpen() const { return !glfwWindowShouldClose(m_window); }
    bool isResized() const { return m_resized; }
    bool isMinimized() const { return m_minimized; }

    bool isKeyPressed(int key) const { return m_keys[key]; }
    bool isKeyJustPressed(int key) const { return m_keys[key] && !m_keysPrev[key]; }
    bool isMouseButtonPressed(int button) const { return m_mouseButtons[button]; }

    const std::vector<unsigned int>& getInputCodepoints() const { return m_inputCodepoints; }

    glm::vec2 getMousePos() const { return m_mousePos; }
    glm::vec2 getMousePosFb() const {
        return glm::vec2(
            m_mousePos.x * (static_cast<float>(m_fbWidth) / m_winWidth),
            m_mousePos.y * (static_cast<float>(m_fbHeight) / m_winHeight));
    }
    glm::vec2 getMouseDelta() const { return m_mouseDelta; }
    glm::vec2 getScrollDelta() const { return m_scrollDelta; }

    const char* getClipboardString() const { return glfwGetClipboardString(m_window); }
    void setClipboardString(const char* text) {
        glfwSetClipboardString(m_window, text);
    }

    int getWidth() const { return m_fbWidth; }
    int getHeight() const { return m_fbHeight; }
    float getAspect() const { return static_cast<float>(m_fbWidth) / m_fbHeight; }

    float getCurrentTime() const { return glfwGetTime(); }
    float getDeltaTime() const { return m_deltaTime; }

    void createSurface(VkInstance instance, VkSurfaceKHR* surface);

private:
    GLFWwindow* m_window = nullptr;

    int m_winWidth = 0;
    int m_winHeight = 0;

    int m_fbWidth = 0;
    int m_fbHeight = 0;

    bool m_resized = false;
    bool m_minimized = false;

    float m_lastFrame = 0.0f;
    float m_deltaTime = 0.0f;

    std::array<bool, GLFW_KEY_LAST + 1> m_keys = {false};
    std::array<bool, GLFW_KEY_LAST + 1> m_keysPrev = {false};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_mouseButtons = {false};

    std::vector<unsigned int> m_inputCodepoints;

    glm::vec2 m_mousePos{0.0f};
    glm::vec2 m_mouseDelta{0.0f};
    glm::vec2 m_scrollDelta{0.0f};
    bool m_firstMouse = true;

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void mousePosCallback(GLFWwindow* window, double x, double y);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void winResizeCallback(GLFWwindow* window, int width, int height);
    static void fbResizeCallback(GLFWwindow* window, int width, int height);
};

} // namespace win