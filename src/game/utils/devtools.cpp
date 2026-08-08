#include "devtools.hpp"

#include <cstring>

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

    m_imgui.addGUI(
        {"Properties", [this]() {
             if (m_selectedInstance) {
                 char nameBuffer[128];
                 std::strncpy(nameBuffer, m_selectedInstance->getName().c_str(), sizeof(nameBuffer));
                 nameBuffer[sizeof(nameBuffer) - 1] = '\0';

                 if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
                     m_selectedInstance->setName(nameBuffer);
                 }

                 ImGui::Separator();

                 ImGui::Text("Type: %d", static_cast<int>(m_selectedInstance->getType()));

                 ImGui::Separator();

                 auto parent = m_selectedInstance->getParent();
                 ImGui::Text("Parent: %s", parent ? parent->getName().c_str() : "None");
                 ImGui::Text("Direct Children: %zu", m_selectedInstance->getChildren().size());
                 ImGui::Text("Total Descendants: %zu", m_selectedInstance->getDescendants().size());

                 ImGui::Separator();

                 if (ImGui::Button("Destroy Instance", ImVec2(-1, 0))) {
                     m_selectedInstance->destroy();
                     m_selectedInstance = nullptr;
                 }
             } else {
                 ImGui::TextDisabled("Select an instance from the Explorer to edit properties");
             }
         }});
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

    if (isOpened && !children.empty()) {
        for (const auto& child : children) {
            drawNode(child);
        }
        ImGui::TreePop();
    }
}

} // namespace game::utils