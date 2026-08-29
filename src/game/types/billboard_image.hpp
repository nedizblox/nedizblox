#pragma once

#include "instance.hpp"

#include <glm/glm.hpp>

namespace game::types {

class BillboardImage : public TypedInstance<BillboardImage> {
public:
    BillboardImage(const std::string& name = "BillboardImage") :
        TypedInstance<BillboardImage>(enums::InstanceType::BillboardImage, name) {}

    static std::shared_ptr<BillboardImage> create(const std::string& name = "BillboardImage") {
        return std::make_shared<BillboardImage>(name);
    }

    std::vector<PropertyDescriptor> getProperties() override {
        std::weak_ptr<BillboardImage> self = std::static_pointer_cast<BillboardImage>(shared_from_this());

        auto instanceProps = Instance::getProperties();
        std::vector<PropertyDescriptor> billbTextProps = {
            {"Offset", "Billboard Image 3D offset of position", enums::PropertyType::Vector3,
             static_cast<void*>(&m_offset), false,
             [self]() {
                 if (auto instance = self.lock())
                     instance->propertyChanged();
             }},
            {"Scale", "Billboard Image 2D scale", enums::PropertyType::Vector2, static_cast<void*>(&m_scale), false,
             [self]() {
                 if (auto instance = self.lock())
                     instance->propertyChanged();
             }},
            {"Source", "Billboard Image source", enums::PropertyType::String, static_cast<void*>(&m_source), false,
             [self]() {
                 if (auto instance = self.lock())
                     instance->propertyChanged();
             }},
        };

        instanceProps.insert(
            instanceProps.end(), std::make_move_iterator(billbTextProps.begin()),
            std::make_move_iterator(billbTextProps.end()));

        return instanceProps;
    }

    glm::vec3 getOffset() const { return m_offset; }
    void setOffset(const glm::vec3& offset) {
        m_offset = offset;
        propertyChanged();
    }

    glm::vec2 getScale() const { return m_scale; }
    void setScale(const glm::vec2& scale) {
        m_scale = scale;
        propertyChanged();
    }

    std::string getSource() const { return m_source; }
    void setSource(const std::string& source) {
        m_source = source;
        propertyChanged();
    }

private:
    glm::vec3 m_offset{0.0f};
    glm::vec2 m_scale{200.0f};

    std::string m_source = "";
};

} // namespace game::types