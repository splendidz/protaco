// Grade.enum.h. protacoc 0.6.0, source Grade.enum.yaml (sha1 4a4ddba23d94). Do not edit.
#pragma once

#include <cstdint>
#include <string>

namespace maps
{

/// Dish grade.
/// [enum; version 1; source Grade.enum.yaml]
enum class Grade : int32_t
{
    Gold = 1,
    Silver = 2,
    Bronze = 3,
};

/// Name of a value, removed values included. nullptr for a number never declared.
inline const char* EnumName(Grade value)
{
    switch (static_cast<int32_t>(value))
    {
    case 1:
        return "Gold";
    case 2:
        return "Silver";
    case 3:
        return "Bronze";
    default:
        return nullptr;
    }
}

/// Value from its name (values of version 1). False, out unchanged and error set for another name.
inline bool TryParseEnum(const std::string& name, Grade* out, std::string* error = nullptr)
{
    if (name == "Gold")
    {
        *out = Grade::Gold;
        return true;
    }
    if (name == "Silver")
    {
        *out = Grade::Silver;
        return true;
    }
    if (name == "Bronze")
    {
        *out = Grade::Bronze;
        return true;
    }
    if (error != nullptr)
        *error = "unknown Grade name: " + name;
    return false;
}

}  // namespace maps
