// PizzaSize.enum.h. protacoc 0.7.0, source PizzaSize.enum.yaml (sha1 ba3536e7746d). Do not edit.
#pragma once

#include <cstdint>
#include <string>

namespace enums
{

/// Pizza size. Kids was dropped in version 3; its number stays reserved and its name still prints.
/// [enum; version 3; source PizzaSize.enum.yaml]
enum class PizzaSize : int32_t
{
    /// 10 inch
    Small = 1,
    Medium = 2,
    Large = 3,
    /// [added in 2]
    Family = 4,
};

/// Name of a value, removed values included. nullptr for a number never declared.
inline const char* EnumName(PizzaSize value)
{
    switch (static_cast<int32_t>(value))
    {
    case 1:
        return "Small";
    case 2:
        return "Medium";
    case 3:
        return "Large";
    case 5:
        return "Kids";
    case 4:
        return "Family";
    default:
        return nullptr;
    }
}

/// Value from its name (values of version 3). False, out unchanged and error set for another name.
inline bool TryParseEnum(const std::string& name, PizzaSize* out, std::string* error = nullptr)
{
    if (name == "Small")
    {
        *out = PizzaSize::Small;
        return true;
    }
    if (name == "Medium")
    {
        *out = PizzaSize::Medium;
        return true;
    }
    if (name == "Large")
    {
        *out = PizzaSize::Large;
        return true;
    }
    if (name == "Family")
    {
        *out = PizzaSize::Family;
        return true;
    }
    if (error != nullptr)
        *error = "unknown PizzaSize name: " + name;
    return false;
}

}  // namespace enums
