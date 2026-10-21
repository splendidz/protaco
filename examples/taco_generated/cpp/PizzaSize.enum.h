// PizzaSize.enum.h. protacoc 0.7.0, source PizzaSize.enum.yaml (sha1 1b92f7597053). Do not edit.
#pragma once

#include <cstdint>
#include <string>

namespace demo
{

/// Pizza size. Values are int32 in the serialized data; ToString prints Large(3).
/// [enum; version 1; source PizzaSize.enum.yaml]
enum class PizzaSize : int32_t
{
    /// 10 inch
    Small = 1,
    /// 12 inch
    Medium = 2,
    /// 14 inch
    Large = 3,
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
    default:
        return nullptr;
    }
}

/// Value from its name (values of version 1). False, out unchanged and error set for another name.
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
    if (error != nullptr)
        *error = "unknown PizzaSize name: " + name;
    return false;
}

}  // namespace demo
