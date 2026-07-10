#pragma once

namespace game::enums {

enum class InstanceType {
    Workspace = 0,
    CoreGui = 1,
    Model = 2,
    Part = 3,
    BillboardText = 4,
    Text = 5
};

enum class PartType { Ball = 0, Block = 1, Cylinder = 2, Wedge = 3, Head = 4 };

} // namespace game::enums