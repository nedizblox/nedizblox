#include "devtools.hpp"

#include <cstring>

#include <imgui/ImGuiFileDialog.h>

#include <glm/gtc/type_ptr.hpp>

#include "../types/factory.hpp"

#include "../utils/nbx_file.hpp"

namespace game::ui {

namespace {

constexpr const char* kOpenDialogKey = "DevToolsOpenMap";
constexpr const char* kSaveDialogKey = "DevToolsSaveMap";
constexpr const char* kMapExtension = ".nbxl";

constexpr float kGizmoTranslateSnap = 1.0f;

void mergeInto(
    const std::shared_ptr<types::Instance>& target,
    const std::shared_ptr<types::Instance>& source
) {
    for (const auto& loadedChild : source->getChildren()) {
        std::shared_ptr<types::Instance> match = nullptr;
        for (const auto& existing : target->getChildren()) {
            if (existing->getType() == loadedChild->getType() &&
                existing->getName() == loadedChild->getName()) {
                match = existing;
                break;
            }
        }

        if (!match) {
            loadedChild->setParent(target);
            continue;
        }

        for (const auto& old : match->getChildren())
            old->destroy();

        for (const auto& moved : loadedChild->getChildren())
            moved->setParent(match);
    }
}

} // namespace

DevTools::DevTools(
    gfx::mngrs::UiManager& uiManager, std::shared_ptr<types::Game>& root,
    utils::Controller& controller, physics::Physics& physics,
    mngrs::HumanoidManager& humanoids
) :
    m_root(root), m_controller(controller), m_physics(physics),
    m_humanoids(humanoids) {
    createGUIs(uiManager);
}

DevTools::~DevTools() {
}

void DevTools::createGUIs(gfx::mngrs::UiManager& uiManager) {
    uiManager.addImguiWindow(
        {"Explorer",
         [this]()
         {
             if (m_root) {
                 drawNode(m_root, true);
             } else {
                 ImGui::TextDisabled("No root instance found");
             }
         },
         ImGuiWindowFlags_NoCollapse, &m_showExplorer,
         gfx::mngrs::UiManager::DockSlot::Right, 0.5f}
    );

    uiManager.addImguiWindow(
        {"Properties", [this]() { drawPropertiesPanel(); },
         ImGuiWindowFlags_NoCollapse, &m_showProperties,
         gfx::mngrs::UiManager::DockSlot::Right}
    );

    uiManager.setImguiMenuBar([this]() { drawMenuBar(); });
    uiManager.setImguiOverlay([this]() { drawOverlay(); });
}

void DevTools::drawNode(
    const std::shared_ptr<types::Instance>& node, bool defaultOpen
) {
    if (!node)
        return;

    auto children = node->getChildren();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (defaultOpen) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (m_selectedInstance == node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    std::string label = node->getName() + " ##" +
                        std::to_string(reinterpret_cast<uintptr_t>(node.get()));
    bool isOpened = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        m_selectedInstance = node;
    }

    if (ImGui::BeginDragDropSource()) {
        types::Instance* raw = node.get();
        ImGui::SetDragDropPayload("DEVTOOLS_INSTANCE", &raw, sizeof(raw));
        ImGui::TextUnformatted(node->getName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("DEVTOOLS_INSTANCE")) {
            auto* raw = *static_cast<types::Instance**>(payload->Data);
            if (raw && raw != node.get()) {
                raw->setParent(node);
            }
        }
        ImGui::EndDragDropTarget();
    }

    bool deleteRequested = false;
    bool createRequested = false;

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("New")) {
            createRequested = true;
        }
        if (ImGui::MenuItem("Delete")) {
            deleteRequested = true;
        }
        ImGui::EndPopup();
    }

    if (createRequested) {
        m_newInstanceParent = node;
        m_openCreateInstancePopup = true;
    }

    if (isOpened && !children.empty() && !deleteRequested) {
        for (const auto& child : children) {
            drawNode(child);
        }
    }

    if (isOpened && !children.empty()) {
        ImGui::TreePop();
    }

    if (deleteRequested) {
        if (m_selectedInstance) {
            if (m_selectedInstance == node) {
                m_selectedInstance = nullptr;
            } else {
                auto descendants = node->getDescendants();
                if (std::find(
                        descendants.begin(), descendants.end(),
                        m_selectedInstance
                    ) != descendants.end()) {
                    m_selectedInstance = nullptr;
                }
            }
        }

        node->destroy();
    }
}

void DevTools::drawPropertiesPanel() {
    if (!m_selectedInstance) {
        ImGui::TextDisabled(
            "Select an instance from the Explorer to edit properties"
        );
        return;
    }

    if (m_propertyDraftsOwner != m_selectedInstance.get()) {
        m_propertyDrafts.clear();
        m_propertyDraftsOwner = m_selectedInstance.get();
    }

    auto properties = m_selectedInstance->getProperties();
    for (auto& prop : properties) {
        drawProperty(prop);
    }
}

void DevTools::drawProperty(types::Instance::PropertyDescriptor& prop) {
    ImGui::PushID(prop.name.c_str());

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(prop.name.c_str());
    if (!prop.description.empty() && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", prop.description.c_str());
    }

    ImGui::SameLine(120.0f);
    ImGui::SetNextItemWidth(-1);

    if (prop.readOnly) {
        ImGui::BeginDisabled();
    }

    ImGuiID draftId = ImGui::GetID("##value");
    PropertyDraft& draft = m_propertyDrafts[draftId];

    switch (prop.type) {
        case enums::PropertyType::String: {
            auto* value = static_cast<std::string*>(prop.ptr);

            char buffer[256];
            if (!draft.active) {
                draft.strValue = *value;
            }
            std::strncpy(buffer, draft.strValue.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';

            ImGui::InputText("##value", buffer, sizeof(buffer));
            draft.strValue = buffer;
            draft.active = ImGui::IsItemActive();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                *value = draft.strValue;
                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Bool: {
            auto* value = static_cast<bool*>(prop.ptr);
            if (ImGui::Checkbox("##value", value) && prop.notifyChanged) {
                prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Int32: {
            auto* value = static_cast<int32_t*>(prop.ptr);

            if (!draft.active) {
                draft.intValue = *value;
            }
            ImGui::InputInt("##value", &draft.intValue);
            draft.active = ImGui::IsItemActive();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                *value = draft.intValue;
                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::UInt32: {
            auto* value = static_cast<uint32_t*>(prop.ptr);

            if (!draft.active) {
                draft.intValue = static_cast<int32_t>(*value);
            }
            ImGui::InputInt("##value", &draft.intValue);
            draft.intValue = glm::max(draft.intValue, 0);
            draft.active = ImGui::IsItemActive();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                *value = static_cast<uint32_t>(draft.intValue);
                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Float: {
            auto* value = static_cast<float*>(prop.ptr);

            if (!draft.active) {
                draft.floatValue[0] = *value;
            }
            ImGui::InputFloat("##value", &draft.floatValue[0]);
            draft.active = ImGui::IsItemActive();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                *value = draft.floatValue[0];
                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Vector3: {
            float* value = reinterpret_cast<float*>(prop.ptr);

            if (!draft.active) {
                draft.floatValue[0] = value[0];
                draft.floatValue[1] = value[1];
                draft.floatValue[2] = value[2];
            }
            ImGui::InputFloat3("##value", draft.floatValue);
            draft.active = ImGui::IsItemActive();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                value[0] = draft.floatValue[0];
                value[1] = draft.floatValue[1];
                value[2] = draft.floatValue[2];
                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Vector2: {
            float* value = reinterpret_cast<float*>(prop.ptr);

            if (!draft.active) {
                draft.floatValue[0] = value[0];
                draft.floatValue[1] = value[1];
            }
            ImGui::InputFloat2("##value", draft.floatValue);
            draft.active = ImGui::IsItemActive();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                value[0] = draft.floatValue[0];
                value[1] = draft.floatValue[1];
                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Color3: {
            auto* value = static_cast<glm::u8vec3*>(prop.ptr);

            if (!draft.active) {
                draft.floatValue[0] = value->x / 255.0f;
                draft.floatValue[1] = value->y / 255.0f;
                draft.floatValue[2] = value->z / 255.0f;
            }

            ImGui::ColorEdit3("##value", draft.floatValue);
            draft.active = ImGui::IsItemActive();

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                *value = glm::u8vec3(
                    static_cast<uint8_t>(
                        glm::clamp(draft.floatValue[0], 0.0f, 1.0f) * 255.0f
                    ),
                    static_cast<uint8_t>(
                        glm::clamp(draft.floatValue[1], 0.0f, 1.0f) * 255.0f
                    ),
                    static_cast<uint8_t>(
                        glm::clamp(draft.floatValue[2], 0.0f, 1.0f) * 255.0f
                    )
                );

                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Enum: {
            auto* value = reinterpret_cast<int32_t*>(prop.ptr);

            if (!prop.enumOptions.empty()) {
                int current = *value;
                if (current < 0 ||
                    current >= static_cast<int>(prop.enumOptions.size())) {
                    current = 0;
                }

                if (ImGui::BeginCombo(
                        "##value", prop.enumOptions[current].c_str()
                    )) {
                    for (int i = 0;
                         i < static_cast<int>(prop.enumOptions.size()); i++) {
                        bool isSelected = (i == current);
                        if (ImGui::Selectable(
                                prop.enumOptions[i].c_str(), isSelected
                            )) {
                            *value = i;
                            if (prop.notifyChanged)
                                prop.notifyChanged();
                        }
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            } else {
                if (ImGui::InputInt("##value", value) && prop.notifyChanged) {
                    prop.notifyChanged();
                }
            }
            break;
        }

        case enums::PropertyType::InstanceRef: {
            auto current =
                prop.getInstanceRef ? prop.getInstanceRef() : nullptr;
            ImGui::Button(
                current ? current->getName().c_str() : "None", ImVec2(-1, 0)
            );

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload =
                        ImGui::AcceptDragDropPayload("DEVTOOLS_INSTANCE")) {
                    auto* raw = *static_cast<types::Instance**>(payload->Data);
                    if (raw && prop.setInstanceRef) {
                        prop.setInstanceRef(raw->shared_from_this());
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (!prop.readOnly && current) {
                ImGui::SameLine();
                if (ImGui::SmallButton("x") && prop.setInstanceRef) {
                    prop.setInstanceRef(nullptr);
                }
            }
            break;
        }
    }

    if (prop.readOnly) {
        ImGui::EndDisabled();
    }

    ImGui::PopID();
}

void DevTools::drawMenuBar() {
    handleShortcuts();

    const bool editing = !m_playing;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open", "Ctrl+O", false, editing)) {
            openMap();
        }

        if (ImGui::MenuItem("Save", "Ctrl+S", false, editing)) {
            saveMap();
        }

        if (ImGui::MenuItem("Save as...", "Ctrl+Shift+S", false, editing)) {
            saveMapAs();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            // TODO
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Editor")) {
        if (ImGui::MenuItem("Copy", "Ctrl+C", false, canCopy())) {
            copySelected();
        }

        if (ImGui::MenuItem("Paste", "Ctrl+V", false, canPaste())) {
            pasteClipboard();
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
                "Move", "F1", m_gizmoOperation == ImGuizmo::TRANSLATE
            )) {
            m_gizmoOperation = ImGuizmo::TRANSLATE;
        }

        if (ImGui::MenuItem(
                "Rotate", "F2", m_gizmoOperation == ImGuizmo::ROTATE
            )) {
            m_gizmoOperation = ImGuizmo::ROTATE;
        }

        if (ImGui::MenuItem(
                "Scale", "F3", m_gizmoOperation == ImGuizmo::SCALE
            )) {
            m_gizmoOperation = ImGuizmo::SCALE;
        }

        ImGui::Separator();

        ImGui::MenuItem("Explorer", nullptr, &m_showExplorer);
        ImGui::MenuItem("Properties", nullptr, &m_showProperties);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Place")) {
        if (ImGui::MenuItem("Play", "F5", false, !m_playing)) {
            play();
        }

        if (ImGui::MenuItem("Stop", "F6", false, m_playing)) {
            stop();
        }

        ImGui::EndMenu();
    }
}

bool DevTools::keyPressedEdge(ImGuiKey key, SDL_Scancode scancode) {
    const int id = static_cast<int>(scancode);

    bool& wasDown = m_rawKeyWasDown[id];
    const bool isDown = m_controller.isKeyDown(scancode);
    const bool rawEdge = isDown && !wasDown;
    wasDown = isDown;

    if (!rawEdge && !ImGui::IsKeyPressed(key, false))
        return false;

    int& lastFrame = m_lastFiredFrame[id];
    const int frame = ImGui::GetFrameCount();
    if (lastFrame != 0 && frame - lastFrame < 3)
        return false;

    lastFrame = frame;
    return true;
}

void DevTools::handleShortcuts() {
    ImGuiIO& io = ImGui::GetIO();

    const bool ctrl = io.KeyCtrl ||
                      m_controller.isKeyDown(SDL_SCANCODE_LCTRL) ||
                      m_controller.isKeyDown(SDL_SCANCODE_RCTRL);
    const bool shift = io.KeyShift ||
                       m_controller.isKeyDown(SDL_SCANCODE_LSHIFT) ||
                       m_controller.isKeyDown(SDL_SCANCODE_RSHIFT);

    const bool pressedO = keyPressedEdge(ImGuiKey_O, SDL_SCANCODE_O);
    const bool pressedS = keyPressedEdge(ImGuiKey_S, SDL_SCANCODE_S);
    const bool pressedC = keyPressedEdge(ImGuiKey_C, SDL_SCANCODE_C);
    const bool pressedV = keyPressedEdge(ImGuiKey_V, SDL_SCANCODE_V);

    if (io.WantTextInput || ImGuiFileDialog::Instance()->IsOpened())
        return;

    if (ctrl && pressedO && !m_playing) {
        openMap();
    }

    if (ctrl && pressedS && !m_playing) {
        if (shift)
            saveMapAs();
        else
            saveMap();
    }

    if (ctrl && pressedC && canCopy()) {
        copySelected();
    }

    if (ctrl && pressedV && canPaste()) {
        pasteClipboard();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
        m_gizmoOperation = ImGuizmo::TRANSLATE;
    } else if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
        m_gizmoOperation = ImGuizmo::ROTATE;
    } else if (ImGui::IsKeyPressed(ImGuiKey_F3, false)) {
        m_gizmoOperation = ImGuizmo::SCALE;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
        play();
    } else if (ImGui::IsKeyPressed(ImGuiKey_F6, false)) {
        stop();
    }
}

void DevTools::drawOverlay() {
    drawCreateInstancePopup();
    drawFileDialogs();
    drawErrorPopup();
    drawGizmo();
}

void DevTools::showError(const std::string& message) {
    m_errorMessage = message;
    m_openErrorPopup = true;
}

void DevTools::drawErrorPopup() {
    if (m_openErrorPopup) {
        ImGui::OpenPopup("Error");
        m_openErrorPopup = false;
    }

    if (ImGui::BeginPopupModal(
            "Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize
        )) {
        ImGui::TextWrapped("%s", m_errorMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void DevTools::resetEditorState() {
    m_selectedInstance = nullptr;
    m_newInstanceParent = nullptr;
    m_openCreateInstancePopup = false;

    m_propertyDrafts.clear();
    m_propertyDraftsOwner = nullptr;

    m_gizmoWasUsing = false;
    m_gizmoHasPendingCommit = false;
}

void DevTools::openMap() {
    if (m_playing)
        return;

    IGFD::FileDialogConfig config;
    config.path =
        m_currentFilePath.empty()
            ? "."
            : std::filesystem::path(m_currentFilePath).parent_path().string();
    config.flags = ImGuiFileDialogFlags_Modal;

    ImGuiFileDialog::Instance()->OpenDialog(
        kOpenDialogKey, "Open map", kMapExtension, config
    );
}

void DevTools::saveMap() {
    if (m_playing)
        return;

    if (m_currentFilePath.empty()) {
        saveMapAs();
        return;
    }

    try {
        writeMap(m_currentFilePath);
    }
    catch (const std::exception& e) {
        showError(e.what());
    }
}

void DevTools::saveMapAs() {
    if (m_playing)
        return;

    IGFD::FileDialogConfig config;
    if (m_currentFilePath.empty()) {
        config.path = ".";
        config.fileName = std::string("map") + kMapExtension;
    } else {
        std::filesystem::path current(m_currentFilePath);
        config.path = current.parent_path().string();
        config.fileName = current.filename().string();
    }
    config.flags =
        ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;

    ImGuiFileDialog::Instance()->OpenDialog(
        kSaveDialogKey, "Save map as", kMapExtension, config
    );
}

void DevTools::drawFileDialogs() {
    ImGuiFileDialog* dialog = ImGuiFileDialog::Instance();
    const ImVec2 minSize(700.0f, 400.0f);

    if (dialog->Display(kOpenDialogKey, ImGuiWindowFlags_NoCollapse, minSize)) {
        if (dialog->IsOk()) {
            const std::string path = dialog->GetFilePathName();
            try {
                loadMap(path);
                m_currentFilePath = path;
            }
            catch (const std::exception& e) {
                showError(e.what());
            }
        }
        dialog->Close();
    }

    if (dialog->Display(kSaveDialogKey, ImGuiWindowFlags_NoCollapse, minSize)) {
        if (dialog->IsOk()) {
            std::string path = dialog->GetFilePathName();
            if (std::filesystem::path(path).extension() != kMapExtension)
                path += kMapExtension;

            try {
                writeMap(path);
                m_currentFilePath = path;
            }
            catch (const std::exception& e) {
                showError(e.what());
            }
        }
        dialog->Close();
    }
}

void DevTools::writeMap(const std::string& path) {
    if (!m_root)
        throw std::runtime_error("No root instance to save");

    utils::nbxf::saveNbxl(path, {m_root});
}

void DevTools::loadMap(const std::string& path) {
    if (!m_root)
        throw std::runtime_error("No root instance to load into");

    auto loaded = utils::nbxf::parseNbxl(path);

    std::vector<std::shared_ptr<types::Instance>> loadedRoots;
    for (const auto& instance : loaded) {
        if (!instance->getParent())
            loadedRoots.push_back(instance);
    }

    resetEditorState();

    for (const auto& loadedRoot : loadedRoots) {
        if (loadedRoot->getType() == m_root->getType())
            mergeInto(m_root, loadedRoot);
        else
            loadedRoot->setParent(m_root);
    }
}

bool DevTools::canCopy() const {
    return !m_playing && m_selectedInstance && m_selectedInstance->getParent();
}

bool DevTools::canPaste() const {
    return !m_playing && !m_clipboard.empty() && m_root;
}

void DevTools::copySelected() {
    if (!canCopy())
        return;

    try {
        m_clipboard = utils::nbxf::serializeTree(m_selectedInstance);
    }
    catch (const std::exception& e) {
        showError(e.what());
    }
}

void DevTools::pasteClipboard() {
    if (!canPaste())
        return;

    try {
        auto copy = utils::nbxf::instantiateTree(m_clipboard);
        if (!copy) {
            showError("Failed to paste: clipboard is empty or invalid");
            return;
        }

        std::shared_ptr<types::Instance> parent = nullptr;
        if (m_selectedInstance)
            parent = m_selectedInstance->getParent();
        if (!parent)
            parent = m_selectedInstance;
        if (!parent)
            parent = m_root;

        copy->setParent(parent);
        m_selectedInstance = copy;
    }
    catch (const std::exception& e) {
        showError(e.what());
    }
}

void DevTools::play() {
    if (m_playing)
        return;

    m_physics.start(true);

    m_localRig = m_humanoids.createHumanoid("Player", 0, true);
    if (!m_localRig) {
        m_physics.stop();
        showError("Failed to create the local player");
        return;
    }

    m_gizmoWasUsing = false;
    m_gizmoHasPendingCommit = false;

    m_controller.setLocalRig(m_localRig);
    m_controller.setFreeCameraMode(false);

    m_playing = true;
}

void DevTools::stop() {
    if (!m_playing)
        return;

    if (m_localRig) {
        auto belongsToRig = [this](const std::shared_ptr<types::Instance>& i)
        {
            if (!i)
                return false;
            if (i == m_localRig)
                return true;
            for (const auto& d : m_localRig->getDescendants()) {
                if (d == i)
                    return true;
            }
            return false;
        };

        if (belongsToRig(m_selectedInstance))
            m_selectedInstance = nullptr;
        if (belongsToRig(m_newInstanceParent))
            m_newInstanceParent = nullptr;
    }

    m_humanoids.removeHumanoid(0);
    m_localRig.reset();

    m_controller.setLocalRig(nullptr);
    m_controller.setFreeCameraMode(true);

    m_physics.stop();

    m_playing = false;
}

void DevTools::drawCreateInstancePopup() {
    if (m_openCreateInstancePopup) {
        ImGui::OpenPopup("Create Instance");
        m_openCreateInstancePopup = false;
    }

    if (ImGui::BeginPopupModal(
            "Create Instance", nullptr, ImGuiWindowFlags_AlwaysAutoResize
        )) {
        if (!m_newInstanceParent) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        ImGui::Text(
            "Create new instance inside \"%s\"",
            m_newInstanceParent->getName().c_str()
        );
        ImGui::Separator();

        ImGui::BeginChild("##CreateInstanceList", ImVec2(220.0f, 200.0f), true);
        for (const auto& option : types::factory::kCreatableTypes) {
            if (ImGui::Selectable(option.name)) {
                auto newInstance = option.create(option.name);
                newInstance->setParent(m_newInstanceParent);

                m_selectedInstance = newInstance;

                m_newInstanceParent = nullptr;
                ImGui::EndChild();
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Cancel")) {
            m_newInstanceParent = nullptr;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

bool DevTools::manipulate(glm::mat4& transform, ImGuizmo::OPERATION operation) {
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);

    glm::mat4 view = m_controller.getCameraView();
    glm::mat4 projection = m_controller.getCameraProjection();

    projection[1][1] *= -1.0f;

    const float translateSnap[3] = {
        kGizmoTranslateSnap, kGizmoTranslateSnap, kGizmoTranslateSnap
    };
    const float* snap =
        (operation == ImGuizmo::TRANSLATE) ? translateSnap : nullptr;

    return ImGuizmo::Manipulate(
        glm::value_ptr(view), glm::value_ptr(projection), operation,
        ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr, snap
    );
}

void DevTools::drawGizmo() {
    if (!m_selectedInstance || m_playing)
        return;

    switch (m_selectedInstance->getType()) {
        case enums::InstanceType::Part:
            drawPartGizmo(
                std::static_pointer_cast<types::Part>(m_selectedInstance)
            );
            break;
        case enums::InstanceType::Model:
            drawModelGizmo(
                std::static_pointer_cast<types::Model>(m_selectedInstance)
            );
            break;
        default:
            break;
    }
}

void DevTools::drawPartGizmo(const std::shared_ptr<types::Part>& part) {
    glm::mat4 transform = part->getModelMatrix();

    bool manipulated = manipulate(transform, m_gizmoOperation);

    if (manipulated) {
        glm::vec3 newPosition = glm::vec3(transform[3]);

        glm::vec3 col0 = glm::vec3(transform[0]);
        glm::vec3 col1 = glm::vec3(transform[1]);
        glm::vec3 col2 = glm::vec3(transform[2]);

        glm::vec3 newSize(
            glm::length(col0), glm::length(col1), glm::length(col2)
        );

        glm::mat3 rotationMatrix(
            newSize.x > 0.0f ? col0 / newSize.x : col0,
            newSize.y > 0.0f ? col1 / newSize.y : col1,
            newSize.z > 0.0f ? col2 / newSize.z : col2
        );
        glm::quat newOrientation = glm::quat_cast(rotationMatrix);

        part->setPosition(newPosition, true);
        part->setOrientation(newOrientation, true);

        if (m_gizmoOperation == ImGuizmo::SCALE) {
            part->setSize(newSize, true);
        }

        m_gizmoHasPendingCommit = true;
        m_gizmoActiveOperation = m_gizmoOperation;
    }

    bool isUsing = ImGuizmo::IsUsing();

    if (m_gizmoWasUsing && !isUsing && m_gizmoHasPendingCommit) {
        part->setPosition(part->getPosition());
        part->setOrientation(part->getOrientation());

        if (m_gizmoActiveOperation == ImGuizmo::SCALE) {
            part->setSize(part->getSize());
        }

        m_gizmoHasPendingCommit = false;
    }

    m_gizmoWasUsing = isUsing;
}

void DevTools::drawModelGizmo(const std::shared_ptr<types::Model>& model) {
    if (m_gizmoOperation == ImGuizmo::SCALE) {
        m_gizmoWasUsing = false;
        m_gizmoHasPendingCommit = false;
        return;
    }

    glm::mat4 transform = model->getPivot();

    bool manipulated = manipulate(transform, m_gizmoOperation);

    if (manipulated) {
        glm::vec3 col0 = glm::vec3(transform[0]);
        glm::vec3 col1 = glm::vec3(transform[1]);
        glm::vec3 col2 = glm::vec3(transform[2]);

        if (glm::length(col0) > 1e-6f && glm::length(col1) > 1e-6f &&
            glm::length(col2) > 1e-6f) {
            glm::quat rotation = glm::quat_cast(
                glm::mat3(
                    glm::normalize(col0), glm::normalize(col1),
                    glm::normalize(col2)
                )
            );

            glm::mat4 pivot = glm::mat4_cast(rotation);
            pivot[3] = glm::vec4(glm::vec3(transform[3]), 1.0f);

            model->setPivot(pivot, true);

            m_gizmoHasPendingCommit = true;
            m_gizmoActiveOperation = m_gizmoOperation;
        }
    }

    bool isUsing = ImGuizmo::IsUsing();

    if (m_gizmoWasUsing && !isUsing && m_gizmoHasPendingCommit) {
        for (const auto& descendant : model->getDescendants()) {
            if (descendant->getType() != enums::InstanceType::Part)
                continue;

            auto part = std::static_pointer_cast<types::Part>(descendant);
            part->setPosition(part->getPosition());
            part->setOrientation(part->getOrientation());
        }

        m_gizmoHasPendingCommit = false;
    }

    m_gizmoWasUsing = isUsing;
}

} // namespace game::ui