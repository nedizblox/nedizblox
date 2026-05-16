#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace game::types {

class Instance : public std::enable_shared_from_this<Instance> {
public:
    Instance(enums::InstanceType type, const std::string& name) : m_type(type), m_name(name) {}
    virtual ~Instance() = default;

    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    enums::InstanceType getType() const { return m_type; }

    Instance* getParent() const { return m_parent; }
    void setParent(Instance* parent) {
        if (m_parent == parent)
            return;

        if (m_parent != nullptr) {
            auto& siblings = m_parent->m_children;
            siblings.erase(
                std::remove_if(
                    siblings.begin(), siblings.end(),
                    [this](const std::shared_ptr<Instance>& child) { return child.get() == this; }),
                siblings.end());
        }

        m_parent = parent;

        if (m_parent != nullptr) {
            m_parent->m_children.push_back(shared_from_this());
        }
    }

    const std::vector<std::shared_ptr<Instance>>& getChildren() const { return m_children; }

    std::shared_ptr<Instance> findFirstChild(const std::string& childName) {
        for (auto& child : m_children) {
            if (child->getName() == childName) {
                return child;
            }
        }

        return nullptr;
    }

private:
    std::string m_name;
    enums::InstanceType m_type;

    Instance* m_parent = nullptr;
    std::vector<std::shared_ptr<Instance>> m_children;
};

} // namespace game::types