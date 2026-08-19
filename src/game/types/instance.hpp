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
    using CallbackId = size_t;

    struct PropertyDescriptor {
        std::string name;
        std::string description;
        enums::PropertyType type;
        void* ptr;
        bool readOnly = false;
 
        std::function<void()> notifyChanged;
 
        std::function<std::shared_ptr<Instance>()> getInstanceRef;
        std::function<void(const std::shared_ptr<Instance>&)> setInstanceRef;

        std::vector<std::string> enumOptions;
    };

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

    virtual std::vector<PropertyDescriptor> getProperties() {
        auto self = weak_from_this();
 
        return {
            {
                "Name",
                "Instance name",
                enums::PropertyType::String,
                static_cast<void*>(&m_name),
                false,
                [self]() {
                    if (auto instance = self.lock())
                        instance->propertyChanged();
                },
            },
            {
                "NetworkId",
                "Network identifier",
                enums::PropertyType::UInt32,
                static_cast<void*>(&m_networkId),
                true,
                nullptr,
            },
            {
                "Parent",
                "Instance parent",
                enums::PropertyType::InstanceRef,
                nullptr,
                false,
                nullptr,
                [self]() -> std::shared_ptr<Instance> {
                    if (auto instance = self.lock())
                        return instance->getParent();
                    return nullptr;
                },
                [self](const std::shared_ptr<Instance>& newParent) {
                    if (auto instance = self.lock())
                        instance->setParent(newParent);
                },
            }
        };
    }

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

    CallbackId onChildrenChanged(Callback callback) {
        CallbackId id = m_nextCallbackId++;
        m_propertyChangedCallbacks.push_back({id, callback});
        return id;
    }

    CallbackId onPropertyChanged(Callback callback) {
        CallbackId id = m_nextCallbackId++;
        m_propertyChangedCallbacks.push_back({id, callback});
        return id;
    }

    CallbackId onDescendantPropertyChanged(Callback callback) {
        CallbackId id = m_nextCallbackId++;
        m_descendantPropertyChangedCallbacks.push_back({id, callback});
        return id;
    }

    CallbackId onDestroy(Callback callback) {
        CallbackId id = m_nextCallbackId++;
        m_destroyCallbacks.push_back({id, callback});
        return id;
    }

    CallbackId onDescendantDestroy(Callback callback) {
        CallbackId id = m_nextCallbackId++;
        m_descendantDestroyCallbacks.push_back({id, callback});
        return id;
    }

    void removeChildrenChangedCallback(CallbackId id) {
        std::erase_if(m_childrenChangedCallbacks, [id](const auto& item) {
            return item.id == id;
        });
    }

    void removePropertyChangedCallback(CallbackId id) {
        std::erase_if(m_propertyChangedCallbacks, [id](const auto& item) {
            return item.id == id;
        });
    }

    void removeDescendantPropertyChangedCallback(CallbackId id) {
        std::erase_if(m_descendantPropertyChangedCallbacks, [id](const auto& item) {
            return item.id == id;
        });
    }

    void removeDestroyCallback(CallbackId id) {
        std::erase_if(m_destroyCallbacks, [id](const auto& item) {
            return item.id == id;
        });
    }

    void removeDescendantDestroyCallback(CallbackId id) {
        std::erase_if(m_descendantDestroyCallbacks, [id](const auto& item) {
            return item.id == id;
        });
    }

    void destroy() {
        auto self = shared_from_this();

        for (const auto& callback : m_destroyCallbacks) {
            if (callback.callback) {
                callback.callback(self);
            }
        }

        if (auto parent = m_parent.lock()) {
            parent->descendantDestroy(self);
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
    struct CallbackEntry {
        CallbackId id;
        Callback callback;
    };

    void propertyChanged() {
        auto self = weak_from_this().lock();

        if (self) {
            for (const auto& entry : m_propertyChangedCallbacks) {
                if (entry.callback) {
                    entry.callback(self);
                }
            }

            if (auto parent = m_parent.lock()) {
                parent->descendantPropertyChanged(self);
            }
        }
    }

    void descendantPropertyChanged(const std::shared_ptr<Instance>& changedInstance) {
        for (const auto& entry : m_descendantPropertyChangedCallbacks) {
            if (entry.callback) {
                entry.callback(changedInstance);
            }
        }

        auto parent = m_parent.lock();
        if (parent) {
            parent->descendantPropertyChanged(changedInstance);
        }
    }

    void descendantDestroy(const std::shared_ptr<Instance>& destroyedInstance) {
        for (const auto& entry : m_descendantDestroyCallbacks) {
            if (entry.callback) {
                entry.callback(destroyedInstance);
            }
        }

        if (auto parent = m_parent.lock()) {
            parent->descendantDestroy(destroyedInstance);
        }
    }

    void childrenChanged() {
        auto self = weak_from_this().lock();

        if (self) {
            for (const auto& entry : m_childrenChangedCallbacks) {
                if (entry.callback) {
                    entry.callback(self);
                }
            }
        }
    }

private:
    std::string m_name;
    enums::InstanceType m_type;

    std::weak_ptr<Instance> m_parent;
    std::vector<std::shared_ptr<Instance>> m_children;

    uint32_t m_networkId = 0;

    std::vector<CallbackEntry> m_childrenChangedCallbacks;
    std::vector<CallbackEntry> m_propertyChangedCallbacks;
    std::vector<CallbackEntry> m_descendantPropertyChangedCallbacks;
    std::vector<CallbackEntry> m_destroyCallbacks;
    std::vector<CallbackEntry> m_descendantDestroyCallbacks;

    inline static CallbackId m_nextCallbackId = 0;

    void collectDescendants(std::vector<std::shared_ptr<Instance>>& result) const {
        for (const auto& child : m_children) {
            if (child) {
                result.push_back(child);
                child->collectDescendants(result);
            }
        }
    }
};

template <typename Derived>
class TypedInstance : public Instance {
public:
    using Instance::Instance;

    using Callback = std::function<void(std::shared_ptr<Derived>)>;

    using Instance::onChildrenChanged;
    using Instance::onPropertyChanged;
    using Instance::onDestroy;

    CallbackId onChildrenChanged(Callback callback) {
        return Instance::onChildrenChanged(wrap(std::move(callback)));
    }

    CallbackId onPropertyChanged(Callback callback) {
        return Instance::onPropertyChanged(wrap(std::move(callback)));
    }

    CallbackId onDestroy(Callback callback) {
        return Instance::onDestroy(wrap(std::move(callback)));
    }

private:
    static Instance::Callback wrap(Callback callback) {
        return [callback = std::move(callback)](const std::shared_ptr<Instance>& instance) {
            if (auto casted = std::static_pointer_cast<Derived>(instance)) {
                callback(casted);
            }
        };
    }
};

} // namespace game::types