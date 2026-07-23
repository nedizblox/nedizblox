#pragma once

namespace game::enums {

enum class InstanceType {
    Workspace = 0,
    CoreGui = 1,
    Model = 2,
    Part = 3,
    Decal = 4,
    BillboardText = 5,
    Text = 6
};

enum class PartType { Ball = 0, Block = 1, Cylinder = 2, Wedge = 3, Head = 4 };

enum class Face { Top = 0, Bottom = 1, Left = 2, Right = 3, Front = 4, Back = 5 };

enum class SurfaceType { Smooth = 0, Studs = 1, Inlets = 2, Glue = 3 };

enum class RigMoveDirection { Forward = 0, Backward = 1, Left = 2, Right = 3 };

enum class TextFont { Nunito = 0, Arial = 1 };

} // namespace game::enums