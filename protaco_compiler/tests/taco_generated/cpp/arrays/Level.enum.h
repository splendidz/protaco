// Level.enum.h. protacoc 0.7.0, source Level.enum.yaml (sha1 50e0c504e666). Do not edit.
#pragma once

#include <cstdint>
#include <string>

namespace arrays
{

/// Alarm level of one slot.
/// [enum; version 1; source Level.enum.yaml]
enum class Level : int32_t
{
    Low = 1,
    High = 2,
    /// number of levels and an array length
    Count = 3,
};

/// Name of a value, removed values included. nullptr for a number never declared.
inline const char* EnumName(Level value)
{
    switch (static_cast<int32_t>(value))
    {
    case 1:
        return "Low";
    case 2:
        return "High";
    case 3:
        return "Count";
    default:
        return nullptr;
    }
}

/// Value from its name (values of version 1). False, out unchanged and error set for another name.
inline bool TryParseEnum(const std::string& name, Level* out, std::string* error = nullptr)
{
    if (name == "Low")
    {
        *out = Level::Low;
        return true;
    }
    if (name == "High")
    {
        *out = Level::High;
        return true;
    }
    if (name == "Count")
    {
        *out = Level::Count;
        return true;
    }
    if (error != nullptr)
        *error = "unknown Level name: " + name;
    return false;
}

}  // namespace arrays
