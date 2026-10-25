#pragma once

#include "../types/instance.hpp"

#include <btBulletDynamicsCommon.h>

namespace game::physics {

class BodyMotionState : public btMotionState {
public:
    ~BodyMotionState() override = default;

    bool isPendingDestroy() const {
        return m_pendingDestroy;
    }

    virtual types::Instance* getTarget() const = 0;

    virtual void detach() = 0;

protected:
    static constexpr float kKillHeight = -500.0f;

    bool m_pendingDestroy = false;
};

} // namespace game::physics