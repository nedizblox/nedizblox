#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/ext.hpp>

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
    void resetScrollDelta() { m_scrollDelta = glm::vec2(0.0f); }

    GLFWwindow* getWindow() const { return m_window; }

    bool isOpen() const { return !glfwWindowShouldClose(m_window); }
    bool isResized() const { return m_resized; }
    bool isMinimized() const { return m_minimized; }

    bool isKeyPressed(int key) const { return m_keys[key]; }
    bool isKeyJustPressed(int key) const { return m_keys[key] && !m_keysPrev[key]; }
    bool isMouseButtonPressed(int button) const { return m_mouseButtons[button]; }

    glm::vec2 getMousePos() const { return m_mousePos; }
    glm::vec2 getMouseRel() const { return m_mouseRel; }
    glm::vec2 getScrollDelta() const { return m_scrollDelta; }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspect() const { return static_cast<float>(m_width) / m_height; }

    float getCurrentTime() const { return glfwGetTime(); }
    float getDeltaTime() const { return m_deltaTime; }

    VkExtent2D getExtent() const;

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    bool m_resized = false;
    bool m_minimized = false;

    float m_lastFrame;
    float m_deltaTime;

    std::array<bool, GLFW_KEY_LAST + 1> m_keys = {false};
    std::array<bool, GLFW_KEY_LAST + 1> m_keysPrev = {false};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_mouseButtons = {false};

    glm::vec2 m_mousePos;
    glm::vec2 m_mouseRel;
    glm::vec2 m_scrollDelta;
    bool m_firstMouse = true;

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void mousePosCallback(GLFWwindow* window, double x, double y);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void resizeCallback(GLFWwindow* window, int width, int height);
};

} // namespace win