#include "window.hpp"

#include <SDL2/SDL_syswm.h>
#include <stb/stb_image.h>

#include <stdexcept>

#include "core/logger.hpp"

namespace win {

Window::Window(int width, int height, const std::string& title) :
    m_winWidth(width), m_winHeight(height), m_fbWidth(width),
    m_fbHeight(height) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(
            std::string("SDL: Failed to initialize: ") + SDL_GetError()
        );
    }

    SDL_LogSetOutputFunction(logOutputFunction, nullptr);

    Uint32 windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

    m_window = SDL_CreateWindow(
        title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        m_fbWidth, m_fbHeight, windowFlags
    );

    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error(
            std::string("SDL: Failed to create window: ") + SDL_GetError()
        );
    }

    SDL_StartTextInput();

#ifndef NDEBUG
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(m_window, &wmInfo)) {
        switch (wmInfo.subsystem) {
            case SDL_SYSWM_WINDOWS:
                core::logger::debug("Using WSI platform Win32");
                break;
            case SDL_SYSWM_WAYLAND:
                core::logger::debug("Using WSI platform Wayland");
                break;
            case SDL_SYSWM_X11:
                core::logger::debug("Using WSI platform X11");
                break;
            case SDL_SYSWM_COCOA:
                core::logger::debug("Using WSI platform Cocoa");
                break;
            default:
                core::logger::warn("Failed to fetch WSI platform");
                break;
        }
    } else {
        core::logger::warn("Failed to fetch WSI platform");
    }
#endif

    m_lastFrame = SDL_GetPerformanceCounter();
}

Window::~Window() {
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Window::setIcon(const std::string& filePath) {
    int width, height, channels;
    stbi_uc* pixels =
        stbi_load(filePath.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        throw std::runtime_error(
            "STB: Failed to open texture image file: " + filePath
        );
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        pixels, width, height, 32, width * 4, 0x000000FF, 0x0000FF00,
        0x00FF0000, 0xFF000000
    );

    if (!surface) {
        stbi_image_free(pixels);
        throw std::runtime_error(
            std::string("SDL: Failed to create icon surface: ") + SDL_GetError()
        );
    }

    SDL_SetWindowIcon(m_window, surface);

    SDL_FreeSurface(surface);
    stbi_image_free(pixels);
}

void Window::update() {
    m_mouseDelta = glm::vec2(0.0f);
    m_scrollDelta = glm::vec2(0.0f);
    m_keysPrev = m_keys;

    m_inputCodepoints.clear();
    m_pollEvents.clear();

    Uint64 currentFrame = SDL_GetPerformanceCounter();
    m_deltaTime = static_cast<float>(currentFrame - m_lastFrame) /
                  static_cast<float>(SDL_GetPerformanceFrequency());
    m_lastFrame = currentFrame;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        m_pollEvents.push_back(event);

        switch (event.type) {
            case SDL_QUIT:
                m_shouldClose = true;
                break;

            case SDL_KEYDOWN:
                if (!event.key.repeat) {
                    m_keys[event.key.keysym.scancode] = true;
                }
                break;

            case SDL_KEYUP:
                m_keys[event.key.keysym.scancode] = false;
                break;

            case SDL_TEXTINPUT: {
                const char* text = event.text.text;
                size_t len = SDL_strlen(text);
                size_t i = 0;
                while (i < len) {
                    unsigned char c = static_cast<unsigned char>(text[i]);
                    unsigned int codepoint = 0;
                    int extra = 0;

                    if (c < 0x80) {
                        codepoint = c;
                        extra = 0;
                    } else if ((c & 0xE0) == 0xC0) {
                        codepoint = c & 0x1F;
                        extra = 1;
                    } else if ((c & 0xF0) == 0xE0) {
                        codepoint = c & 0x0F;
                        extra = 2;
                    } else if ((c & 0xF8) == 0xF0) {
                        codepoint = c & 0x07;
                        extra = 3;
                    }

                    i++;
                    for (int k = 0; k < extra && i < len; k++, i++) {
                        codepoint =
                            (codepoint << 6) |
                            (static_cast<unsigned char>(text[i]) & 0x3F);
                    }

                    m_inputCodepoints.push_back(codepoint);
                }
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button < m_mouseButtons.size()) {
                    m_mouseButtons[event.button.button] = true;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button < m_mouseButtons.size()) {
                    m_mouseButtons[event.button.button] = false;
                }
                break;

            case SDL_MOUSEMOTION:
                if (m_firstMouse) {
                    m_mousePos = glm::vec2(
                        static_cast<float>(event.motion.x),
                        static_cast<float>(event.motion.y)
                    );
                    m_firstMouse = false;
                } else {
                    m_mouseDelta += glm::vec2(
                        static_cast<float>(event.motion.xrel),
                        -static_cast<float>(event.motion.yrel)
                    );
                    m_mousePos = glm::vec2(
                        static_cast<float>(event.motion.x),
                        static_cast<float>(event.motion.y)
                    );
                }
                break;

            case SDL_MOUSEWHEEL:
                m_scrollDelta += glm::vec2(
                    static_cast<float>(event.wheel.x),
                    static_cast<float>(event.wheel.y)
                );
                break;

            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        m_winWidth = event.window.data1;
                        m_winHeight = event.window.data2;
                        break;

                    case SDL_WINDOWEVENT_CLOSE:
                        m_shouldClose = true;
                        break;

                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }

    int fbWidth, fbHeight;
    SDL_Vulkan_GetDrawableSize(m_window, &fbWidth, &fbHeight);

    m_resized = (fbWidth != m_fbWidth || fbHeight != m_fbHeight);
    m_fbWidth = fbWidth;
    m_fbHeight = fbHeight;

    m_minimized = (fbWidth == 0 || fbHeight == 0);
}

void Window::createSurface(VkInstance instance, VkSurfaceKHR* surface) {
    if (SDL_Vulkan_CreateSurface(m_window, instance, surface) != SDL_TRUE) {
        throw std::runtime_error(
            std::string("Vulkan: Failed to create window surface: ") +
            SDL_GetError()
        );
    }
}

void Window::getInstanceExtensions(
    unsigned int* extensionCount, const char** extensions
) {
    if (SDL_Vulkan_GetInstanceExtensions(
            m_window, extensionCount, extensions
        ) != SDL_TRUE) {
        throw std::runtime_error(
            std::string("SDL: Failed to query instance extensions: ") +
            SDL_GetError()
        );
    }
}

void Window::spawnMessageBox(
    const std::string& title, const std::string& message,
    SDL_MessageBoxFlags flags
) {
    if (SDL_ShowSimpleMessageBox(
            flags, title.c_str(), message.c_str(), nullptr
        ) != 0) {
        core::logger::err(SDL_GetError());
    }
}

void Window::logOutputFunction(
    void* userdata, int category, SDL_LogPriority priority, const char* message
) {
    if (priority >= SDL_LOG_PRIORITY_ERROR) {
        core::logger::err("SDL: ", message);
    } else if (priority >= SDL_LOG_PRIORITY_WARN) {
        core::logger::warn("SDL: ", message);
    } else {
        core::logger::info("SDL: ", message);
    }
}

} // namespace win
