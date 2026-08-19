#include "devtools.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <glm/glm.hpp>

namespace game::utils {

DevTools::DevTools(gfx::ui::Imgui& imgui, std::shared_ptr<types::Game>& root) :
    m_imgui(imgui), m_root(root) {
    createGUIs();
}

DevTools::~DevTools() {}

void DevTools::createGUIs() {
    m_imgui.addGUI({"Explorer", [this]() {
                        if (m_root) {
                            drawNode(m_root);
                        } else {
                            ImGui::TextDisabled("No root instance found");
                        }
                    }});

    m_imgui.addGUI({"Properties", [this]() { drawPropertiesPanel(); }});
}

void DevTools::drawNode(const std::shared_ptr<types::Instance>& node) {
    if (!node)
        return;

    auto children = node->getChildren();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (m_selectedInstance == node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    std::string label = node->getName() + " ##" + std::to_string(reinterpret_cast<uintptr_t>(node.get()));
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
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DEVTOOLS_INSTANCE")) {
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
                if (std::find(descendants.begin(), descendants.end(), m_selectedInstance) != descendants.end()) {
                    m_selectedInstance = nullptr;
                }
            }
        }

        node->destroy();
    }
}

void DevTools::drawPropertiesPanel() {
    if (!m_selectedInstance) {
        ImGui::TextDisabled("Select an instance from the Explorer to edit properties");
        return;
    }

    auto instanceTypeNames = enums::getStringsFromEnums<enums::InstanceType>();
    int instanceTypeIndex = static_cast<int>(m_selectedInstance->getType());
    const char* instanceTypeName =
        (instanceTypeIndex >= 0 && instanceTypeIndex < static_cast<int>(instanceTypeNames.size()))
            ? instanceTypeNames[instanceTypeIndex].c_str()
            : "Unknown";

    ImGui::Text("Type: %s", instanceTypeName);
    ImGui::Text("Direct Children: %zu", m_selectedInstance->getChildren().size());
    ImGui::Text("Total Descendants: %zu", m_selectedInstance->getDescendants().size());

    ImGui::Separator();

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

    switch (prop.type) {
        case enums::PropertyType::String: {
            auto* value = static_cast<std::string*>(prop.ptr);

            char buffer[256];
            std::strncpy(buffer, value->c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';

            if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
                *value = buffer;
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
            if (ImGui::InputInt("##value", value) && prop.notifyChanged) {
                prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::UInt32: {
            auto* value = static_cast<uint32_t*>(prop.ptr);
            int temp = static_cast<int>(*value);
            if (ImGui::InputInt("##value", &temp)) {
                *value = static_cast<uint32_t>(std::max(temp, 0));
                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Float: {
            auto* value = static_cast<float*>(prop.ptr);
            if (ImGui::DragFloat("##value", value, 0.05f) && prop.notifyChanged) {
                prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Vector3: {
            float* value = reinterpret_cast<float*>(prop.ptr);
            if (ImGui::DragFloat3("##value", value, 0.05f) && prop.notifyChanged) {
                prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Vector2: {
            float* value = reinterpret_cast<float*>(prop.ptr);
            if (ImGui::DragFloat2("##value", value, 0.05f) && prop.notifyChanged) {
                prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Color3: {
            auto* value = static_cast<glm::u8vec3*>(prop.ptr);
            float color[3] = {value->x / 255.0f, value->y / 255.0f, value->z / 255.0f};

            if (ImGui::ColorEdit3("##value", color)) {
                *value = glm::u8vec3(
                    static_cast<uint8_t>(std::clamp(color[0], 0.0f, 1.0f) * 255.0f),
                    static_cast<uint8_t>(std::clamp(color[1], 0.0f, 1.0f) * 255.0f),
                    static_cast<uint8_t>(std::clamp(color[2], 0.0f, 1.0f) * 255.0f));

                if (prop.notifyChanged)
                    prop.notifyChanged();
            }
            break;
        }

        case enums::PropertyType::Enum: {
            auto* value = reinterpret_cast<int32_t*>(prop.ptr);

            if (!prop.enumOptions.empty()) {
                int current = *value;
                if (current < 0 || current >= static_cast<int>(prop.enumOptions.size())) {
                    current = 0;
                }

                if (ImGui::BeginCombo("##value", prop.enumOptions[current].c_str())) {
                    for (int i = 0; i < static_cast<int>(prop.enumOptions.size()); i++) {
                        bool isSelected = (i == current);
                        if (ImGui::Selectable(prop.enumOptions[i].c_str(), isSelected)) {
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
            auto current = prop.getInstanceRef ? prop.getInstanceRef() : nullptr;
            ImGui::Button(current ? current->getName().c_str() : "None", ImVec2(-1, 0));

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DEVTOOLS_INSTANCE")) {
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

} // namespace game::utils