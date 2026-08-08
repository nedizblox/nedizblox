#pragma once

#include "../types/types.hpp"

#include "graphics/ui/imgui.hpp"

namespace game::utils {

class DevTools {
public:
    DevTools(gfx::ui::Imgui& imgui, std::shared_ptr<types::Game>& root);
    ~DevTools();

    DevTools(const DevTools&) = delete;
    DevTools& operator=(const DevTools&) = delete;

private:
    gfx::ui::Imgui& m_imgui;

    std::shared_ptr<types::Game>& m_root;

    std::shared_ptr<types::Instance> m_selectedInstance = nullptr;

    void createGUIs();

    void drawNode(const std::shared_ptr<types::Instance>& node);
};

} // namespace game::utils