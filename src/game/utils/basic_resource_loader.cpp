#include "basic_resource_loader.hpp"

namespace game::utils {

void brl::load(
    gfx::mngrs::ModelManager& modelManager, gfx::mngrs::UiManager& uiManager, win::Window& window,
    gfx::mngrs::BillboardManager& billboardManager, mngrs::AssetManager& assetManager) {
    modelManager.loadSkybox();

    modelManager.loadModel("cube", gfx::geoms::cube);
    modelManager.loadModel("sphere", gfx::geoms::sphere);
    modelManager.loadModel("cylinder", gfx::geoms::cylinder);
    modelManager.loadModel("wedge", gfx::geoms::wedge);

    modelManager.loadModelOutline("cube", gfx::geoms::cube);
    modelManager.loadModelOutline("sphere", gfx::geoms::sphere);
    modelManager.loadModelOutline("cylinder", gfx::geoms::cylinder);
    modelManager.loadModelOutline("wedge", gfx::geoms::wedge);

    modelManager.loadModel("head", "assets/models/head.obj");

    assetManager.loadTexture("blank", "assets/textures/blank.png");

    assetManager.loadTexture("smooth", "assets/textures/smooth.png");
    assetManager.loadTexture("studs", "assets/textures/studs.png");
    assetManager.loadTexture("inlets", "assets/textures/inlets.png");
    assetManager.loadTexture("glue", "assets/textures/glue.png");

    assetManager.loadTexture("grass", "assets/textures/grass.png");
    assetManager.loadTexture("wood", "assets/textures/wood.png");

    assetManager.loadTexture("headFace", "assets/textures/face.png", "repeat", true, false);
    assetManager.loadTexture("nedizbloxTshirt", "assets/textures/logo_tshirt.png", "repeat", true, false);
    assetManager.loadTexture("spawnLocation", "assets/textures/spawn_location.png", "repeat", true, false);

    const std::array<std::string, 6> skyboxFaces
        = {"assets/textures/skybox/rt.png", "assets/textures/skybox/lf.png",
           "assets/textures/skybox/up.png", "assets/textures/skybox/dn.png",
           "assets/textures/skybox/ft.png", "assets/textures/skybox/bk.png"};
    assetManager.loadCubemap("skybox", skyboxFaces, "skybox");

    uiManager.loadText("nunito", assetManager.getSampler("ui"), "assets/fonts/Nunito.ttf");
    uiManager.loadImgui(window, assetManager.getSampler("ui"));

    billboardManager.loadText("nunito", assetManager.getSampler("repeat"), "assets/fonts/Nunito.ttf");
}

} // namespace game::utils