#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <glm/glm.hpp>

#include <array>
#include <string>
#include <vector>

namespace win {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void update();

    void setTitle(const std::string& title) { SDL_SetWindowTitle(m_window, title.c_str()); }
    void setIcon(const std::string& filePath);

    void resetResizedState() { m_resized = false; }

    bool isOpen() const { return !m_shouldClose; }
    bool isResized() const { return m_resized; }
    bool isMinimized() const { return m_minimized; }

    bool isKeyPressed(SDL_Scancode key) const { return m_keys[key]; }
    bool isKeyJustPressed(SDL_Scancode key) const { return m_keys[key] && !m_keysPrev[key]; }
    bool isMouseButtonPressed(int button) const { return m_mouseButtons[button]; }

    void waitEvents() { SDL_WaitEvent(nullptr); }

    const std::vector<unsigned int>& getInputCodepoints() const { return m_inputCodepoints; }

    glm::vec2 getMousePos() const { return m_mousePos; }
    glm::vec2 getMousePosFb() const {
        return glm::vec2(
            m_mousePos.x * (static_cast<float>(m_fbWidth) / m_winWidth),
            m_mousePos.y * (static_cast<float>(m_fbHeight) / m_winHeight));
    }
    glm::vec2 getMouseDelta() const { return m_mouseDelta; }
    glm::vec2 getScrollDelta() const { return m_scrollDelta; }

    const char* getClipboardString() const { return SDL_GetClipboardText(); }
    void setClipboardString(const char* text) {
        SDL_SetClipboardText(text);
    }

    int getWidth() const { return m_fbWidth; }
    int getHeight() const { return m_fbHeight; }
    float getAspect() const { return static_cast<float>(m_fbWidth) / m_fbHeight; }

    float getCurrentTime() const {
        return static_cast<float>(SDL_GetPerformanceCounter()) / static_cast<float>(SDL_GetPerformanceFrequency());
    }
    float getDeltaTime() const { return m_deltaTime; }

    void createSurface(VkInstance instance, VkSurfaceKHR* surface);

    void getInstanceExtensions(unsigned int* extensionCount, const char** extensions);

    static void spawnMessageBox(const std::string& title, const std::string& message, SDL_MessageBoxFlags flags);

private:
    SDL_Window* m_window = nullptr;

    int m_winWidth = 0;
    int m_winHeight = 0;

    int m_fbWidth = 0;
    int m_fbHeight = 0;

    bool m_resized = false;
    bool m_minimized = false;
    bool m_shouldClose = false;

    Uint64 m_lastFrame = 0;
    float m_deltaTime = 0.0f;

    // Indexed by SDL_Scancode.
    std::array<bool, SDL_NUM_SCANCODES> m_keys = {false};
    std::array<bool, SDL_NUM_SCANCODES> m_keysPrev = {false};
    std::array<bool, 8> m_mouseButtons = {false};

    std::vector<unsigned int> m_inputCodepoints;

    glm::vec2 m_mousePos{0.0f};
    glm::vec2 m_mouseDelta{0.0f};
    glm::vec2 m_scrollDelta{0.0f};
    bool m_firstMouse = true;

    static void logOutputFunction(void* userdata, int category, SDL_LogPriority priority, const char* message);
};

} // namespace win