#pragma once

#include "instance.hpp"

namespace game::types {

class Decal : public Instance {
public:
    Decal(const std::string& name = "Decal") : Instance(enums::InstanceType::Decal, name) {}

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