#pragma once

#include "instance.hpp"

#include "part.hpp"

namespace game::types {

class Model : public TypedInstance<Model> {
public:
    Model(const std::string& name = "Model") : TypedInstance<Model>(enums::InstanceType::Model, name) {}

    glm::mat4 getPivot() const { return m_pivot; }
    virtual void setPivot(const glm::mat4& pivot, bool silent = false) {
        glm::mat4 delta = pivot * glm::inverse(m_pivot);

        m_pivot = pivot;

        updateChildrenTransforms(this, delta);

        if (!silent)
            propertyChanged();
    }

    glm::vec3 getPivotPosition() const { return getPivot()[3]; }
    virtual void setPivotPosition(const glm::vec3& position, bool silent = false) {
        glm::mat4 rotation = m_pivot;
        rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        glm::mat4 pivot = glm::translate(glm::mat4(1.0f), position) * rotation;
        setPivot(pivot, silent);
    }

private:
    glm::mat4 m_pivot{1.0f};

    void updateChildrenTransforms(Instance* parent, const glm::mat4& delta) {
        for (const auto& child : parent->getChildren()) {
            if (child->getType() == enums::InstanceType::Part) {
                auto part = std::static_pointer_cast<Part>(child);

                glm::vec4 newPos = delta * glm::vec4(part->getPosition(), 1.0f);
                part->setPosition(glm::vec3(newPos));

                glm::quat deltaRotation = glm::toQuat(delta);
                part->setOrientation(deltaRotation * part->getOrientation());
            } else if (child->getType() == enums::InstanceType::Model) {
                auto subModel = std::static_pointer_cast<Model>(child);

                subModel->m_pivot = delta * subModel->m_pivot;

                updateChildrenTransforms(subModel.get(), delta);
            }
        }
    }
};

} // namespace game::types