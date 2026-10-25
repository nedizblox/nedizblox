#pragma once

#include "billboard_text.hpp"
#include "decal.hpp"
#include "humanoid.hpp"
#include "model.hpp"

#include <array>

namespace game::types {

class Rig : public Model {
public:
    struct LimbLayout {
        const char* name;
        glm::vec3 offset;
    };

    static const std::array<LimbLayout, 6>& getLimbLayout() {
        static const std::array<LimbLayout, 6> layout = {{
            {"Head", glm::vec3(0.0f, 1.5f, 0.0f)},
            {"Torso", glm::vec3(0.0f, 0.0f, 0.0f)},
            {"LeftArm", glm::vec3(-1.5f, 0.0f, 0.0f)},
            {"RightArm", glm::vec3(1.5f, 0.0f, 0.0f)},
            {"LeftLeg", glm::vec3(-0.5f, -2.0f, 0.0f)},
            {"RightLeg", glm::vec3(0.5f, -2.0f, 0.0f)},
        }};
        return layout;
    }

    Rig(const std::string& name = "Rig") : Model(name) {
    }

    static std::shared_ptr<Rig> create(const std::string& name = "Rig") {
        auto rig = std::make_shared<Rig>(name);

        auto head = Part::create("Head");
        auto torso = Part::create("Torso");
        auto rootPart = Part::create("RootPart");
        auto leftArm = Part::create("LeftArm");
        auto rightArm = Part::create("RightArm");
        auto leftLeg = Part::create("LeftLeg");
        auto rightLeg = Part::create("RightLeg");

        head->setSize(glm::vec3(1.0f, 1.0f, 1.0f));
        torso->setSize(glm::vec3(2.0f, 2.0f, 1.0f));
        rootPart->setSize(glm::vec3(2.0f, 6.0f, 1.0f));
        leftArm->setSize(glm::vec3(1.0f, 2.0f, 1.0f));
        rightArm->setSize(glm::vec3(1.0f, 2.0f, 1.0f));
        leftLeg->setSize(glm::vec3(1.0f, 2.0f, 1.0f));
        rightLeg->setSize(glm::vec3(1.0f, 2.0f, 1.0f));

        head->setColor(glm::u8vec3(255, 255, 0));
        torso->setColor(glm::u8vec3(0, 13, 255));
        leftArm->setColor(glm::u8vec3(255, 255, 0));
        rightArm->setColor(glm::u8vec3(255, 255, 0));
        leftLeg->setColor(glm::u8vec3(0, 98, 255));
        rightLeg->setColor(glm::u8vec3(0, 98, 255));

        rootPart->setTransparency(1.0f);

        head->setShape(enums::PartShape::Head);

        head->setParent(rig);
        torso->setParent(rig);
        rootPart->setParent(rig);
        leftArm->setParent(rig);
        rightArm->setParent(rig);
        leftLeg->setParent(rig);
        rightLeg->setParent(rig);

        auto humanoid = Humanoid::create();
        humanoid->setParent(rig);

        auto face = Decal::create("face");
        face->setSource("headFace");
        face->setFace(enums::Face::Front);
        face->setParent(head);

        auto tshirt = Decal::create("tshirt");
        tshirt->setSource("nedizbloxTshirt");
        tshirt->setFace(enums::Face::Front);
        tshirt->setParent(torso);

        auto nickname = BillboardText::create("nickname");
        nickname->setText(name);
        nickname->setOffset(glm::vec3(0.0f, 0.8f, 0.0f));
        nickname->setParent(head);

        return rig;
    }

    std::shared_ptr<Part> getPart(const std::string& partName) {
        auto child = findFirstChild(partName);
        if (!child || child->getType() != enums::InstanceType::Part)
            return nullptr;
        return std::static_pointer_cast<Part>(child);
    }

    std::shared_ptr<Part> getRootPart() {
        return getPart("RootPart");
    }

    std::shared_ptr<Humanoid> getHumanoid() {
        auto child = findFirstChild("Humanoid");
        if (!child || child->getType() != enums::InstanceType::Humanoid)
            return nullptr;
        return std::static_pointer_cast<Humanoid>(child);
    }

    btRigidBody* getRigidBody() {
        auto root = getRootPart();
        return root ? root->getRigidBody() : nullptr;
    }
};

} // namespace game::types