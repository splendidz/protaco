// Node.g.h. protacoc 0.6.0, source Node.yaml (sha1 0d8cdfcb64fd). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace recursive
{

/// A tree that refers to itself: children in a list, and one optional next node.
/// [version 1; schema 0x94A77C41; source Node.yaml]
class Node : public protaco::CMessage<Node>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Node()
        : Node(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Node(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Node::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Node";
    }

    /// Version of this type. ToObject reads frames of this version or newer as this version.
    static uint16_t MsgVersion()
    {
        return 1;
    }

    /// Oldest supported version (min_version). Frames carry one schema fingerprint per version from here up.
    static uint16_t MsgMinVersion()
    {
        return 1;
    }

    /// Schema fingerprint of this version. A frame whose fingerprint for this version differs is rejected by ToObject.
    static uint32_t MsgSchemaHash()
    {
        return 0x94A77C41u;
    }

    /// [type string]
    const std::string& get_name() const
    {
        return name_;
    }

    void set_name(const std::string& value)
    {
        name_ = value;
        cache_.valid = false;
    }

    /// [type int]
    int32_t get_value() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 0);
    }

    void set_value(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 0, value);
    }

    /// [type list<Node>]
    const std::vector<Node>& get_children() const
    {
        return children_;
    }

    std::vector<Node>& mutable_children()
    {
        cache_.exposed = true;
        return children_;
    }

    void set_children(const std::vector<Node>& value)
    {
        children_ = value;
        cache_.valid = false;
    }

    /// Absent until set. Nothing in the serialized data while absent.
    /// [type Node; optional, absent until set]
    /// get_ returns an empty Node while absent; mutable_ creates it.
    bool has_next() const
    {
        return next_.Has();
    }

    const Node& get_next() const
    {
        return next_.Get();
    }

    Node& mutable_next()
    {
        cache_.exposed = true;
        return next_.Mutable();
    }

    void set_next(const Node& value)
    {
        next_.Set(value);
        cache_.valid = false;
    }

    void clear_next()
    {
        next_.Clear();
        cache_.valid = false;
    }

    /// Extra key-value data not in the yaml. Unknown keys of received messages are kept here.
    const protaco::CExt& GetExt() const
    {
        return ext_;
    }

    protaco::CExt& GetMutableExt()
    {
        return ext_;
    }

    /// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.
    /// Unchanged variable fields and ext reuse the last bytes. After a mutable_ call the variable block is rebuilt every time.
    /// The returned reference is valid until this object is modified.
    const std::vector<uint8_t>& Serialize() const
    {
        return protaco::detail::Access::Serialize(*this);
    }

    /// Text for logs: field names and values, ext entries under ext{...}. protaco::TextStyle::Pretty() for one field per line.
    std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const
    {
        return protaco::detail::Access::ToText(*this, style);
    }

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Node>;

    enum : uint32_t
    {
        kFixedOffset = 40,   // frame header (28 + name 4 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 4,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x94A77C41u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "name", "value", "children", "next" };
        return protaco::detail::InList(key, kKeys, 4);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("name", self.name_, false);
        visitor.Field("children", self.children_, false);
        visitor.Field("next", self.next_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Node& self, TWriter& writer)
    {
        writer.Field("name", self.name_);
        writer.Field("value", self.get_value());
        writer.Field("children", self.children_);
        writer.Field("next", self.next_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::string name_;
    std::vector<Node> children_;
    protaco::detail::Box<Node> next_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace recursive
