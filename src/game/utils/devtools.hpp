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
    using CreatorFunc = std::shared_ptr<types::Instance>(*)(const std::string&);

    struct InstanceTypeOption {
        enums::InstanceType type;
        const char* name;
        CreatorFunc create;
    };

    template <typename T>
    static std::shared_ptr<types::Instance> createInstance(const std::string& name) {
        return T::create(name);
    }

    static constexpr InstanceTypeOption kCreatableTypes[] = {
        {enums::InstanceType::Model, "Model", &createInstance<types::Model>},
        {enums::InstanceType::Folder, "Folder", &createInstance<types::Folder>},
        {enums::InstanceType::Part, "Part", &createInstance<types::Part>},
        {enums::InstanceType::SelectionBox, "SelectionBox", &createInstance<types::SelectionBox>},
        {enums::InstanceType::Decal, "Decal", &createInstance<types::Decal>},
        {enums::InstanceType::BillboardText, "BillboardText", &createInstance<types::BillboardText>},
        {enums::InstanceType::BillboardImage, "BillboardImage", &createInstance<types::BillboardImage>},
        {enums::InstanceType::Text, "Text", &createInstance<types::Text>},
        //{enums::InstanceType::Image, "Image", &createInstance<types::Image>},
    };

    gfx::ui::Imgui& m_imgui;

    std::shared_ptr<types::Game>& m_root;

    std::shared_ptr<types::Instance> m_selectedInstance = nullptr;

    std::shared_ptr<types::Instance> m_newInstanceParent = nullptr;
    bool m_openCreateInstancePopup = false;

    void createGUIs();

    void drawNode(const std::shared_ptr<types::Instance>& node);

    void drawPropertiesPanel();
    void drawProperty(types::Instance::PropertyDescriptor& prop);

    void drawCreateInstancePopup();
};

} // namespace game::utils