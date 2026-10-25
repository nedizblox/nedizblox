#pragma once

#include "../types/types.hpp"

#include "graphics/managers/ui_manager.hpp"

#include "../utils/controller.hpp"

#include "../managers/humanoid_manager.hpp"
#include "../physics/physics.hpp"

#include <imgui/ImGuizmo.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace game::ui {

class DevTools {
public:
    DevTools(
        gfx::mngrs::UiManager& uiManager, std::shared_ptr<types::Game>& root,
        utils::Controller& controller, physics::Physics& physics,
        mngrs::HumanoidManager& humanoids
    );
    ~DevTools();

    DevTools(const DevTools&) = delete;
    DevTools& operator=(const DevTools&) = delete;

private:
    struct PropertyDraft {
        bool active = false;
        std::string strValue;
        float floatValue[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        int32_t intValue = 0;
    };

    std::shared_ptr<types::Game>& m_root;

    utils::Controller& m_controller;
    physics::Physics& m_physics;
    mngrs::HumanoidManager& m_humanoids;

    bool m_showExplorer = true;
    bool m_showProperties = true;

    bool m_playing = false;
    std::shared_ptr<types::Rig> m_localRig = nullptr;

    std::string m_currentFilePath;

    std::vector<uint8_t> m_clipboard;

    std::unordered_map<int, bool> m_rawKeyWasDown;
    std::unordered_map<int, int> m_lastFiredFrame;

    std::string m_errorMessage;
    bool m_openErrorPopup = false;

    std::shared_ptr<types::Instance> m_selectedInstance = nullptr;

    std::shared_ptr<types::Instance> m_newInstanceParent = nullptr;
    bool m_openCreateInstancePopup = false;

    ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::TRANSLATE;

    bool m_gizmoWasUsing = false;
    bool m_gizmoHasPendingCommit = false;
    ImGuizmo::OPERATION m_gizmoActiveOperation = ImGuizmo::TRANSLATE;

    std::unordered_map<ImGuiID, PropertyDraft> m_propertyDrafts;
    types::Instance* m_propertyDraftsOwner = nullptr;

    void createGUIs(gfx::mngrs::UiManager& uiManager);

    void drawNode(
        const std::shared_ptr<types::Instance>& node, bool defaultOpen = false
    );

    void drawPropertiesPanel();
    void drawProperty(types::Instance::PropertyDescriptor& prop);

    void drawMenuBar();
    void handleShortcuts();
    bool keyPressedEdge(ImGuiKey key, SDL_Scancode scancode);

    void drawOverlay();
    void drawCreateInstancePopup();
    void drawFileDialogs();
    void drawErrorPopup();

    void drawGizmo();
    void drawPartGizmo(const std::shared_ptr<types::Part>& part);
    void drawModelGizmo(const std::shared_ptr<types::Model>& model);
    bool manipulate(glm::mat4& transform, ImGuizmo::OPERATION operation);

    void showError(const std::string& message);
    void resetEditorState();

    void openMap();
    void saveMap();
    void saveMapAs();
    void loadMap(const std::string& path);
    void writeMap(const std::string& path);

    bool canCopy() const;
    bool canPaste() const;
    void copySelected();
    void pasteClipboard();

    void play();
    void stop();
};

} // namespace game::ui