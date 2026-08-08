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

    void setName(const std::string& name) {
        m_name = name;
        propertyChanged();
    }

    enums::InstanceType getType() const { return m_type; }

    uint32_t getNetworkId() const { return m_networkId; }

    void setNetworkId(uint32_t networkId) { m_networkId = networkId; }

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

            currentParent->childrenChanged();
        }

        if (newParent) {
            m_parent = newParent;
            newParent->m_children.push_back(self);

            newParent->childrenChanged();
        } else {
            m_parent.reset();
        }
    }

    std::vector<std::shared_ptr<Instance>> getChildren() const { return m_children; }

    std::vector<std::shared_ptr<Instance>> getDescendants() const {
        std::vector<std::shared_ptr<Instance>> descendants;
        collectDescendants(descendants);
        return descendants;
    }

    template <typename T = Instance>
    std::shared_ptr<T> findFirstChild(const std::string& childName) const {
        auto childrenCopy = getChildren();
        for (const auto& child : childrenCopy) {
            if (child && child->getName() == childName) {
                return std::dynamic_pointer_cast<T>(child);
            }
        }
        return nullptr;
    }

    template <typename T>
    std::shared_ptr<T> findFirstChildOfClass() const {
        auto childrenCopy = getChildren();
        for (const auto& child : childrenCopy) {
            if (child) {
                if (auto casted = std::dynamic_pointer_cast<T>(child)) {
                    return casted;
                }
            }
        }
        return nullptr;
    }

    void onChildrenChanged(Callback callback) { m_childrenChangedCallbacks.push_back(callback); }

    void onPropertyChanged(Callback callback) { m_propertyChangedCallbacks.push_back(callback); }

    void onDestroy(Callback callback) { m_destroyCallbacks.push_back(callback); }

    void destroy() {
        auto self = shared_from_this();

        for (const auto& callback : m_destroyCallbacks) {
            if (callback) {
                callback(self);
            }
        }

        for (auto& child : m_children) {
            if (child) {
                child->destroy();
            }
        }

        m_children.clear();

        setParent(nullptr);
    }

protected:
    void propertyChanged() {
        auto self = weak_from_this().lock();

        if (self) {
            for (const auto& callback : m_propertyChangedCallbacks) {
                if (callback) {
                    callback(self);
                }
            }
        }
    }

    void childrenChanged() {
        auto self = weak_from_this().lock();

        if (self) {
            for (const auto& callback : m_childrenChangedCallbacks) {
                if (callback) {
                    callback(self);
                }
            }
        }
    }

private:
    void collectDescendants(std::vector<std::shared_ptr<Instance>>& result) const {
        for (const auto& child : m_children) {
            if (child) {
                result.push_back(child);
                child->collectDescendants(result);
            }
        }
    }

    std::string m_name;
    enums::InstanceType m_type;

    std::weak_ptr<Instance> m_parent;
    std::vector<std::shared_ptr<Instance>> m_children;

    uint32_t m_networkId = 0;

    std::vector<Callback> m_childrenChangedCallbacks;
    std::vector<Callback> m_propertyChangedCallbacks;
    std::vector<Callback> m_destroyCallbacks;
};

} // namespace game::types