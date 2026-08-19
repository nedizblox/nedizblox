#pragma once

#include "instance.hpp"

namespace game::types {

class Decal : public TypedInstance<Decal> {
public:
    Decal(const std::string& name = "Decal") : TypedInstance<Decal>(enums::InstanceType::Decal, name) {}

    std::vector<PropertyDescriptor> getProperties() override {
        std::weak_ptr<Decal> self = std::static_pointer_cast<Decal>(shared_from_this());

        auto instanceProps = Instance::getProperties();
        std::vector<PropertyDescriptor> decalProps = {
            {
                "Source",
                "Decal texture source",
                enums::PropertyType::String,
                static_cast<void*>(&m_source),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                }
            },
            {
                "Face",
                "Decal face",
                enums::PropertyType::Enum,
                static_cast<void*>(&m_face),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                },
                nullptr,
                nullptr,
                enums::getStringsFromEnums<enums::Face>()
            }
        };

        instanceProps.insert(
            instanceProps.end(),
            std::make_move_iterator(decalProps.begin()),
            std::make_move_iterator(decalProps.end()));

        return instanceProps;
    }

    std::string getSource() const { return m_source; }
    void setSource(const std::string& source) {
        m_source = source;
        propertyChanged();
    }

    enums::Face getFace() const { return m_face; }
    void setFace(enums::Face face) {
        m_face = face;
        propertyChanged();
    }

private:
    std::string m_source = "";

    enums::Face m_face = enums::Face::Top;
};

} // namespace game::types