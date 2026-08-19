#pragma once

#include <vector>
#include <string>

namespace game::enums {

enum class InstanceType {
    Game = 0,
    Workspace = 1,
    CoreGui = 2,
    Model = 3,
    Part = 4,
    SelectionBox = 5,
    Decal = 6,
    BillboardText = 7,
    Text = 8
};

enum class PropertyType {
    String = 0,
    Bool = 1,
    Int32 = 2,
    UInt32 = 3,
    Float = 4,
    Vector3 = 5,
    Vector2 = 6,
    Color3 = 7,
    InstanceRef = 8,
    Enum = 9,
};

enum class PartType { Ball = 0, Block = 1, Cylinder = 2, Wedge = 3, Head = 4 };

enum class Face { Top = 0, Bottom = 1, Left = 2, Right = 3, Front = 4, Back = 5 };

enum class SurfaceType { Smooth = 0, Studs = 1, Inlets = 2, Glue = 3 };

enum class MaterialType { SmoothPlastic = 0, Grass = 1, Wood = 2 };

enum class RigMoveDirection { Forward = 0, Backward = 1, Left = 2, Right = 3 };

enum class TextFont { Nunito = 0, Arial = 1 };

template <typename T>
std::vector<std::string> getStringsFromEnums();
 
template <>
inline std::vector<std::string> getStringsFromEnums<InstanceType>() {
    return {"Game", "Workspace", "CoreGui", "Model", "Part", "SelectionBox", "Decal", "BillboardText", "Text"};
}
 
template <>
inline std::vector<std::string> getStringsFromEnums<PartType>() {
    return {"Ball", "Block", "Cylinder", "Wedge", "Head"};
}
 
template <>
inline std::vector<std::string> getStringsFromEnums<Face>() {
    return {"Top", "Bottom", "Left", "Right", "Front", "Back"};
}
 
template <>
inline std::vector<std::string> getStringsFromEnums<SurfaceType>() {
    return {"Smooth", "Studs", "Inlets", "Glue"};
}
 
template <>
inline std::vector<std::string> getStringsFromEnums<MaterialType>() {
    return {"SmoothPlastic", "Grass", "Wood"};
}
 
template <>
inline std::vector<std::string> getStringsFromEnums<RigMoveDirection>() {
    return {"Forward", "Backward", "Left", "Right"};
}
 
template <>
inline std::vector<std::string> getStringsFromEnums<TextFont>() {
    return {"Nunito", "Arial"};
}

} // namespace game::enums