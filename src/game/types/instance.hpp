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
    using Callback = std::function<void(std::shared_ptr<Instance>)>;

    Instance(enums::InstanceType type, const std::string& name) : m_type(type), m_name(name) {}
    virtual ~Instance() = default;

    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    enums::InstanceType getType() const { return m_type; }

    template <typename T = Instance>
    std::shared_ptr<T> getParent() const {
        return std::dynamic_pointer_cast<T>(m_parent.lock());
    }

    void setParent(const std::shared_ptr<Instance>& newParent) {
        auto currentParent = m_parent.lock();
        if (currentParent == newParent)
            return;

        auto self = shared_from_this();

        if (currentParent) {
            auto& siblings = currentParent->m_children;
            siblings.erase(
                std::remove_if(
                    siblings.begin(), siblings.end(),
                    [this](const std::shared_ptr<Instance>& child) { return child.get() == this; }),
                siblings.end());

            if (currentParent->m_childrenChangedCallback) {
                currentParent->m_childrenChangedCallback(currentParent);
            }
        }

        if (newParent) {
            m_parent = newParent;
            newParent->m_children.push_back(self);

            if (newParent->m_childrenChangedCallback) {
                newParent->m_childrenChangedCallback(newParent);
            }
        } else {
            m_parent.reset();
        }
    }

    const std::vector<std::shared_ptr<Instance>>& getChildren() const { return m_children; }

    template <typename T = Instance>
    std::shared_ptr<T> findFirstChild(const std::string& childName) const {
        for (const auto& child : m_children) {
            if (child && child->getName() == childName) {
                return std::dynamic_pointer_cast<T>(child);
            }
        }
        return nullptr;
    }

    template <typename T>
    std::shared_ptr<T> findFirstChildOfClass() const {
        for (const auto& child : m_children) {
            if (child) {
                if (auto casted = std::dynamic_pointer_cast<T>(child)) {
                    return casted;
                }
            }
        }
        return nullptr;
    }

    void onChildrenChanged(Callback callback) { m_childrenChangedCallback = callback; }
    void onPropertyChanged(Callback callback) { m_propertyChangedCallback = callback; }
    void onDestroy(Callback callback) { m_destroyCallback = callback; }

    void destroy() {
        auto self = shared_from_this();

        if (m_destroyCallback) {
            m_destroyCallback(self);
        }

        auto childrenCopy = m_children;
        for (auto& child : childrenCopy) {
            if (child) {
                child->destroy();
            }
        }

        m_children.clear();
        setParent(nullptr);
    }

protected:
    void propertyChanged() {
        if (auto self = weak_from_this().lock()) {
            if (m_propertyChangedCallback) {
                m_propertyChangedCallback(self);
            }
        }
    }

private:
    std::string m_name;
    enums::InstanceType m_type;

    std::weak_ptr<Instance> m_parent;
    std::vector<std::shared_ptr<Instance>> m_children;

    Callback m_childrenChangedCallback;
    Callback m_propertyChangedCallback;
    Callback m_destroyCallback;
};

} // namespace game::types