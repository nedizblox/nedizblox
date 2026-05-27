#pragma once

#include "../enums/enums.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace game::types {

class Instance : public std::enable_shared_from_this<Instance> {
public:
    using ChildCallback = std::function<void(std::shared_ptr<Instance>)>;

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
            auto self = shared_from_this();
            m_parent->m_children.push_back(self);

            if (m_parent->m_childAddedCallback) {
                m_parent->m_childAddedCallback(self);
            }
        }
    }

    const std::vector<std::shared_ptr<Instance>>& getChildren() const { return m_children; }

    template <typename T = Instance>
    std::shared_ptr<T> findFirstChild(const std::string& childName) {
        for (auto& child : m_children) {
            if (child->getName() == childName) {
                return std::dynamic_pointer_cast<T>(child);
            }
        }

        return nullptr;
    }

    void onChildAdded(ChildCallback callback) { m_childAddedCallback = callback; }

private:
    std::string m_name;
    enums::InstanceType m_type;

    Instance* m_parent = nullptr;
    std::vector<std::shared_ptr<Instance>> m_children;

    ChildCallback m_childAddedCallback = nullptr;
};

} // namespace game::types