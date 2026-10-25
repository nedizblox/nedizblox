#include "basic_resource_loader.hpp"

#include "graphics/geometries.hpp"

namespace game::utils {

void brl::load(
    gfx::mngrs::ModelManager& modelManager, gfx::mngrs::UiManager& uiManager,
    win::Window& window, gfx::mngrs::BillboardManager& billboardManager,
    gfx::mngrs::TextureManager& textureManager
) {
    modelManager.loadSkybox();

    modelManager.loadModel("cube", gfx::geoms::cube);
    modelManager.loadModel("sphere", gfx::geoms::sphere);
    modelManager.loadModel("cylinder", gfx::geoms::cylinder);
    modelManager.loadModel("wedge", gfx::geoms::wedge);

    modelManager.loadModelOutline("cube", gfx::geoms::cube);
    modelManager.loadModelOutline("sphere", gfx::geoms::sphere);
    modelManager.loadModelOutline("cylinder", gfx::geoms::cylinder);
    modelManager.loadModelOutline("wedge", gfx::geoms::wedge);

    modelManager.loadModelShadow("cube", gfx::geoms::cube);
    modelManager.loadModelShadow("sphere", gfx::geoms::sphere);
    modelManager.loadModelShadow("cylinder", gfx::geoms::cylinder);
    modelManager.loadModelShadow("wedge", gfx::geoms::wedge);

    modelManager.loadModel("head", "assets/models/head.obj");
    modelManager.loadModelShadow("head", "assets/models/head.obj");

    textureManager.loadTexture("blank", "assets/textures/blank.png");

    textureManager.loadTexture("smooth", "assets/textures/smooth.png");
    textureManager.loadTexture("studs", "assets/textures/studs.png");
    textureManager.loadTexture("inlets", "assets/textures/inlets.png");
    textureManager.loadTexture("glue", "assets/textures/glue.png");

    textureManager.loadTexture("grass", "assets/textures/grass.png");
    textureManager.loadTexture("wood", "assets/textures/wood.png");

    textureManager.loadTexture(
        "headFace", "assets/textures/face.png", "repeat", true, false
    );
    textureManager.loadTexture(
        "nedizbloxTshirt", "assets/textures/logo_tshirt.png", "repeat", true,
        false
    );
    textureManager.loadTexture(
        "spawnLocation", "assets/textures/spawn_location.png", "repeat", true,
        false
    );

    const std::array<std::string, 6> skyboxFaces = {
        "assets/textures/skybox/rt.png", "assets/textures/skybox/lf.png",
        "assets/textures/skybox/up.png", "assets/textures/skybox/dn.png",
        "assets/textures/skybox/ft.png", "assets/textures/skybox/bk.png"
    };
    textureManager.loadCubemap("skybox", skyboxFaces, "skybox");

    uiManager.loadText(
        "nunito", textureManager.getSampler("ui"), "assets/fonts/Nunito.ttf"
    );
    uiManager.loadImgui(window, textureManager.getSampler("ui"));

    billboardManager.loadText(
        "nunito", textureManager.getSampler("repeat"), "assets/fonts/Nunito.ttf"
    );
}

} // namespace game::utils