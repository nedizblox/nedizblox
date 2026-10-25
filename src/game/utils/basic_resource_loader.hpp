#pragma once

#include "graphics/managers/billboard_manager.hpp"
#include "graphics/managers/model_manager.hpp"
#include "graphics/managers/texture_manager.hpp"
#include "graphics/managers/ui_manager.hpp"

namespace game::utils::brl {

void load(
    gfx::mngrs::ModelManager& modelManager, gfx::mngrs::UiManager& uiManager, win::Window& window,
    gfx::mngrs::BillboardManager& billboardManager, gfx::mngrs::TextureManager& textureManager);

} // namespace game::utils::brl