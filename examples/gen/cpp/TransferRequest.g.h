// TransferRequest.g.h. protacoc 0.4.0, source TransferRequest.yaml (sha1 54437178ff1e). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "MoveInfo.g.h"

namespace demo
{

/// Robot transfer request.
/// [version 2; schema 0xCE8A25DA; owner planner; source TransferRequest.yaml]
class TransferRequest
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    TransferRequest()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &TransferRequest::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "TransferRequest";
    }

    /// Version of this type. ToObject reads frames of this version or newer as this version.
    static uint16_t MsgVersion()
    {
        return 2;
    }

    /// Lowest version of the window. Frames carry one schema fingerprint per version from here up.
    static uint16_t MsgMinVersion()
    {
        return 2;
    }

    /// Schema fingerprint of this version. A frame whose fingerprint for this version differs is rejected by ToObject.
    static uint32_t MsgSchemaHash()
    {
        return 0xCE8A25DAu;
    }

    /// robot id
    /// [type string; required, decoding fails if missing]
    const std::string& get_tm() const
    {
        return tm_;
    }

    void set_tm(const std::string& value)
    {
        tm_ = value;
    }

    /// Moves, executed in order.
    /// moves
    /// [type list<MoveInfo>; required, decoding fails if missing]
    const std::vector<MoveInfo>& get_moves() const
    {
        return moves_;
    }

    std::vector<MoveInfo>& get_mutable_moves()
    {
        return moves_;
    }

    void set_moves(const std::vector<MoveInfo>& value)
    {
        moves_ = value;
    }

    /// why the planner chose this
    /// [type string; default ""]
    const std::string& get_reason() const
    {
        return reason_;
    }

    void set_reason(const std::string& value)
    {
        reason_ = value;
    }

    /// Extra key-value data not in the yaml. Unknown keys of received messages are kept here.
    const protaco::CExt& get_ext() const
    {
        return ext_;
    }

    protaco::CExt& get_mutable_ext()
    {
        return ext_;
    }

    /// Writes the envelope (id, corr, ts) straight into the frame header.
    void SetEnvelope(const protaco::Envelope& env)
    {
        protaco::detail::StoreEnvelope(buf_, env);
    }

    protaco::Envelope GetEnvelope() const
    {
        return protaco::detail::LoadEnvelope(buf_.data());
    }

    /// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.
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

    enum : uint32_t
    {
        kFixedOffset = 51,   // frame header (28 + name 15 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 0,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xCE8A25DAu };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "tm", "moves", "reason" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("tm", self.tm_, true);
        visitor.Field("moves", self.moves_, true);
        visitor.Field("reason", self.reason_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const TransferRequest& self, TWriter& writer)
    {
        writer.Field("tm", self.tm_);
        writer.Field("moves", self.moves_);
        writer.Field("reason", self.reason_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::string tm_;
    std::vector<MoveInfo> moves_;
    std::string reason_;
    protaco::CExt ext_;
};

}  // namespace demo
