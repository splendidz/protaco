# protaco.py. Python runtime for protacoc generated code (<Name>_g.py, <Enum>_enum.py). Python 3.8 or newer,
# standard library only. Produces the same bytes as the C++ runtime (Protaco.hpp). Format v4: README.md
# "Serialization format". Strings are UTF-8 on every platform.
# Try* calls return a Result (false on failure, with the reason) and never raise for bad input.
# protacoc writes this file next to the generated packages, so generator and runtime versions always match.
"""protaco runtime: frames, envelopes, ext and ToString for protacoc generated messages."""

import codecs
import datetime
import math
import struct
import threading
import time

VERSION = "0.6.0"

FRAME_VERSION = 4
MAX_DEPTH = 100
MAX_SCHEMA_VERSIONS = 255
MAX_FIELDS = 1024
MAX_FRAME_BYTES = 0xFFFFFFFF

# Value kinds. The numbers are part of the serialized data. Never change them.
BOOL = 1
INT = 2
INT64 = 3
DOUBLE = 4
STRING = 5
MESSAGE = 6
LIST = 7
BYTES = 8
UINT = 9
UINT64 = 10
MAP = 11

_TYPE_NAMES = {BOOL: "bool", INT: "int", INT64: "int64", DOUBLE: "double", STRING: "string", MESSAGE: "message",
               LIST: "list", BYTES: "bytes", UINT: "uint", UINT64: "uint64", MAP: "map"}

TIMESTAMP_MIN = 946684800000000      # 2000-01-01 00:00:00 UTC. Smaller timestamps are not treated as times.
TIMESTAMP_MAX = 253402300799999999   # 9999-12-31 23:59:59.999999 UTC

_U8 = struct.Struct("<B")
_U16 = struct.Struct("<H")
_U32 = struct.Struct("<I")
_ENV = struct.Struct("<IIq")
_SCALARS = {BOOL: struct.Struct("<?"), INT: struct.Struct("<i"), INT64: struct.Struct("<q"), UINT: struct.Struct("<I"),
            UINT64: struct.Struct("<Q"), DOUBLE: struct.Struct("<d")}


# Lone surrogates become U+FFFD, as in the C# runtime.
def _replace_with_fffd(error):
    return ("�", error.end)


codecs.register_error("protaco.replace", _replace_with_fffd)


def _utf8(text):
    return text.encode("utf-8", "protaco.replace")


def _text(data, offset, length):
    return bytes(data[offset:offset + length]).decode("utf-8", "replace")


def _type_name(kind):
    return _TYPE_NAMES.get(kind, "unknown")


class Result:
    """Outcome of a Try* call. True on success; value holds the result, error the reason ("path: reason").
    Unpacks as ok, value, error."""

    __slots__ = ("ok", "value", "error")

    def __init__(self, ok, value=None, error=None):
        self.ok = ok
        self.value = value
        self.error = error

    def __bool__(self):
        return self.ok

    def __iter__(self):
        return iter((self.ok, self.value, self.error))

    def __repr__(self):
        return "Result(ok=%r, value=%r, error=%r)" % (self.ok, self.value, self.error)


class ProtacoError(Exception):
    """Decode or encode error. path looks like "moves[2].from"; str() is "path: reason"."""

    def __init__(self, path, what):
        Exception.__init__(self, what if not path else path + ": " + what)
        self.path = path or ""
        self.what = what

    def _prepend(self, segment):
        if not self.path:
            path = segment
        elif self.path[0] == "[":
            path = segment + self.path
        else:
            path = segment + "." + self.path
        return ProtacoError(path, self.what)


class Envelope:
    """Message envelope, stored in the frame header. id: new id per message (NextId), corr: id of the message this one
    answers (set by the user), ts: UTC microseconds since 1970-01-01 (NowUs). id and ts are set when a message object
    is created."""

    __slots__ = ("id", "corr", "ts")

    def __init__(self, id=0, corr=0, ts=0):
        self.id = id
        self.corr = corr
        self.ts = ts

    def __eq__(self, other):
        return isinstance(other, Envelope) and (self.id, self.corr, self.ts) == (other.id, other.corr, other.ts)

    def __repr__(self):
        return "Envelope(id=%d, corr=%d, ts=%d)" % (self.id, self.corr, self.ts)


class Header:
    """Frame header (PeekHeader)."""

    __slots__ = ("name", "version", "minVersion", "schemaHash", "env", "bodySize")

    def __init__(self):
        self.name = ""
        self.version = 0       # sender's version of the message
        self.minVersion = 0    # oldest version the sender supports; the frame has a fingerprint per version
        self.schemaHash = 0    # fingerprint of version (the sender's MsgSchemaHash)
        self.env = Envelope()
        self.bodySize = 0      # whole frame = header (28 + name + 4 per fingerprint) + bodySize


class _NoStamp:
    __slots__ = ()

    def __repr__(self):
        return "NO_STAMP"


NO_STAMP = _NoStamp()
"""Constructor argument _stamp=NO_STAMP: no envelope id or time (decoded messages, nested fields)."""

_id_lock = threading.Lock()
_last_id = 0


def NextId():
    """Next envelope id of this process: 1, 2, 3, ... and 0 is skipped after wrap-around. Thread-safe."""
    global _last_id
    with _id_lock:
        _last_id = (_last_id + 1) & 0xFFFFFFFF
        if _last_id == 0:
            _last_id = 1
        return _last_id


def _system_clock_us():
    return time.time_ns() // 1000


_clock = _system_clock_us


def NowUs():
    """Current time in UTC microseconds since 1970-01-01 from the clock set by SetClock (default: the system clock)."""
    return _clock()


def SetClock(now_us=None):
    """Replaces the time source of NowUs, for example with a simulated clock. None restores the system clock."""
    global _clock
    _clock = now_us if now_us is not None else _system_clock_us


_auto_envelope = True


def DisableAutoEnvelope():
    """New message objects of this process keep id 0 and ts 0, as with auto_envelope false."""
    global _auto_envelope
    _auto_envelope = False


def EnableAutoEnvelope():
    """Default: a new message object gets id = NextId() and ts = NowUs(), unless its yaml has auto_envelope false."""
    global _auto_envelope
    _auto_envelope = True


def TryGetTimeParts(timestamp_us):
    """Timestamp (UTC microseconds) to local time. Result value: (year, month, day, hour, minute, second, millisecond,
    microsecond). Values outside 2000-01-01..9999-12-31 are not times."""
    if timestamp_us < TIMESTAMP_MIN or timestamp_us > TIMESTAMP_MAX:
        return Result(False, None, "timestamp %d is not a time (2000-01-01 to 9999-12-31 in UTC microseconds)" % timestamp_us)
    try:
        local = datetime.datetime.fromtimestamp(timestamp_us // 1000000)
    except (OverflowError, OSError, ValueError):
        return Result(False, None, "timestamp %d has no local time" % timestamp_us)
    fraction = timestamp_us % 1000000
    return Result(True, (local.year, local.month, local.day, local.hour, local.minute, local.second, fraction // 1000,
                         fraction % 1000))


def TryGetTime(timestamp_us):
    """Timestamp to local "YYYY-MM-DD" and "hh:mm:ss.ffffff". Result value: (date, time)."""
    parts = TryGetTimeParts(timestamp_us)
    if not parts:
        return parts
    year, month, day, hour, minute, second, millisecond, microsecond = parts.value
    return Result(True, ("%04d-%02d-%02d" % (year, month, day),
                         "%02d:%02d:%02d.%06d" % (hour, minute, second, millisecond * 1000 + microsecond)))


class TextStyle:
    """ToString style. Default: one line. Pretty(): one field per line, indented, envelope first."""

    __slots__ = ("separator", "open", "close", "indent", "maxText", "envelope")

    def __init__(self, separator=", ", open="{", close="}", indent=0, maxText=64, envelope=False):
        self.separator = separator   # between fields and list items on one line
        self.open = open
        self.close = close
        self.indent = indent         # spaces per level. 0 = one line
        self.maxText = maxText       # longer strings are cut. Bytes show at most 8 bytes
        self.envelope = envelope     # show envelope{id, corr, ts} of the top-level message first

    @staticmethod
    def Pretty(indent_width=2):
        return TextStyle(indent=indent_width, envelope=True)


# ---- ext ----

class Ext:
    """Extra key-value data not in the yaml. Receivers keep unknown keys here and send them on.
    Kinds: BOOL, INT, INT64, UINT, UINT64, DOUBLE, STRING, BYTES. Keys: 1..255 bytes, not starting with '_', not a field
    key of the message (any version). SetValue and TryGetValue return a Result and never raise."""

    __slots__ = ("_schema_keys", "_open", "_entries")

    def __init__(self, schema_keys, open_):
        self._schema_keys = schema_keys
        self._open = open_
        self._entries = []   # [key, kind, bytes]

    @property
    def Count(self):
        return len(self._entries)

    def Keys(self):
        return [entry[0] for entry in self._entries]

    def Has(self, key):
        return self._index(key) >= 0

    def Remove(self, key):
        index = self._index(key)
        if index < 0:
            return False
        del self._entries[index]
        return True

    def SetValue(self, key, value, kind=None):
        """kind may be left out for bool, float, str and bytes. An int needs INT, INT64, UINT or UINT64."""
        if kind is None:
            if isinstance(value, bool):
                kind = BOOL
            elif isinstance(value, float):
                kind = DOUBLE
            elif isinstance(value, str):
                kind = STRING
            elif isinstance(value, (bytes, bytearray, memoryview)):
                kind = BYTES
            elif isinstance(value, int):
                return Result(False, None, "ext value %d needs a kind: INT, INT64, UINT or UINT64" % value)
            else:
                return Result(False, None, "unsupported ext value type " + type(value).__name__)
        try:
            data = _encode_ext(value, kind)
        except ProtacoError as e:
            return Result(False, None, e.what)
        error = self._check_key(key)
        if error is not None:
            return Result(False, None, error)
        index = self._index(key)
        if index >= 0:
            self._entries[index] = [key, kind, data]
        else:
            self._entries.append([key, kind, data])
        return Result(True)

    def TryGetValue(self, key, kind):
        """Result value: the value. False if the key is missing or holds another kind."""
        if kind not in _SCALARS and kind not in (STRING, BYTES):
            return Result(False, None, "unsupported ext value kind %r" % (kind,))
        index = self._index(key)
        if index < 0:
            return Result(False, None, "ext key not found: " + key)
        _, held, data = self._entries[index]
        if held != kind:
            return Result(False, None, "ext key %s holds %s, not %s" % (key, _type_name(held), _type_name(kind)))
        if kind in _SCALARS:
            scalar = _SCALARS[kind]
            if len(data) != scalar.size:
                return Result(False, None, "ext key %s has a bad %s value" % (key, _type_name(kind)))
            return Result(True, scalar.unpack(data)[0])
        if kind == STRING:
            return Result(True, data.decode("utf-8", "replace"))
        return Result(True, bytes(data))

    def _check_key(self, key):
        if not self._open:
            return "ext is off for this message (allow_extension false)"
        if not isinstance(key, str) or not key:
            return "ext key is empty"
        if len(_utf8(key)) > 255:
            return "ext key is longer than 255 bytes: " + key[:32] + "..."
        if key[0] == "_":
            return "ext key starts with '_' (reserved): " + key
        if key in self._schema_keys:
            return "ext key is a field of the message: " + key
        return None

    def _index(self, key):
        for i, entry in enumerate(self._entries):
            if entry[0] == key:
                return i
        return -1


def _encode_ext(value, kind):
    if kind == STRING:
        if not isinstance(value, str):
            raise ProtacoError("", "%r is not a string" % (value,))
        return _utf8(value)
    if kind == BYTES:
        if value is None:
            return b""
        if not isinstance(value, (bytes, bytearray, memoryview)):
            raise ProtacoError("", "%r is not bytes" % (value,))
        return bytes(value)
    scalar = _SCALARS.get(kind)
    if scalar is None:
        raise ProtacoError("", "unsupported ext value kind %r" % (kind,))
    return _pack(scalar, value, _type_name(kind))


def _pack(scalar, value, kind_name):
    try:
        return scalar.pack(value)
    except (struct.error, TypeError, OverflowError):
        raise ProtacoError("", "%r does not fit %s" % (value, kind_name)) from None


# ---- items: how one value is written, read and shown ----

class Item:
    """One kind of value: field, list element, map key or map value (generated code only, see Items)."""

    __slots__ = ("type", "name", "code", "write", "read", "text", "to_fixed", "from_fixed")

    def __init__(self, type_, name, write, read, text, code=None, to_fixed=None, from_fixed=None):
        self.type = type_
        self.name = name             # yaml type word, for error text
        self.code = code             # struct code in the fixed block; None for variable values
        self.write = write           # (out: bytearray, value) appends the value bytes
        self.read = read             # (data, offset, length, depth) -> value
        self.text = text             # (builder, value)
        self.to_fixed = to_fixed     # (value, key) -> struct argument
        self.from_fixed = from_fixed  # struct result -> value


def _scalar_size(length, expected):
    if length != expected:
        raise ProtacoError("", "scalar size mismatch")


def _scalar_item(kind, code, text):
    scalar = _SCALARS[kind]
    name = _type_name(kind)

    def write(out, value):
        out += _pack(scalar, value, name)

    def read(data, offset, length, depth):
        _scalar_size(length, scalar.size)
        return scalar.unpack_from(data, offset)[0]

    def plain(value, key):
        return value

    return Item(kind, name, write, read, text, code, plain, lambda value: value)


def _write_string(out, value):
    if not isinstance(value, str):
        raise ProtacoError("", "%r is not a string" % (value,))
    out += _utf8(value)


def _write_bytes(out, value):
    if value is None:
        return
    if not isinstance(value, (bytes, bytearray, memoryview)):
        raise ProtacoError("", "%r is not bytes" % (value,))
    out += value


def _begin_sized(out):
    at = len(out)
    out += b"\0\0\0\0"
    return at


def _end_sized(out, at):
    _U32.pack_into(out, at, len(out) - at - 4)


# Len u32 of the next value. None if it runs past end.
def _take_size(data, p, end):
    if end - p < 4:
        return None
    size = _U32.unpack_from(data, p)[0]
    if size > end - p - 4:
        return None
    return size


class Items:
    """Item descriptors of generated messages. Same bytes as the C++ codecs:
    list: ElemType u8 | Count u32 | (Len u32 | Value) x Count,
    map: KeyType u8 | ValueType u8 | Count u32 | (KeyLen u32 | Key | ValueLen u32 | Value) x Count."""

    Bool = _scalar_item(BOOL, "?", lambda b, v: b.bool_value(v))
    Int = _scalar_item(INT, "i", lambda b, v: b.int_value(v))
    Int64 = _scalar_item(INT64, "q", lambda b, v: b.int_value(v))
    UInt = _scalar_item(UINT, "I", lambda b, v: b.int_value(v))
    UInt64 = _scalar_item(UINT64, "Q", lambda b, v: b.int_value(v))
    Double = _scalar_item(DOUBLE, "d", lambda b, v: b.double_value(v))
    String = Item(STRING, "string", _write_string, lambda data, offset, length, depth: _text(data, offset, length),
                  lambda b, v: b.string_value(v))
    Bytes = Item(BYTES, "bytes", _write_bytes, lambda data, offset, length, depth: bytes(data[offset:offset + length]),
                 lambda b, v: b.bytes_value(v))

    @staticmethod
    def Chars(capacity):
        """string(N): fixed block only. UTF-8 bytes, zero padded."""
        def to_fixed(value, key):
            if not isinstance(value, str):
                raise ProtacoError(key, "%r is not a string" % (value,))
            data = _utf8(value)
            if len(data) > capacity:
                raise ProtacoError(key, "%d bytes, longer than string(%d)" % (len(data), capacity))
            return data

        def from_fixed(value):
            return value.split(b"\0", 1)[0].decode("utf-8", "replace")

        return Item(STRING, "string(%d)" % capacity, None, None, lambda b, v: b.string_value(v), "%ds" % capacity,
                    to_fixed, from_fixed)

    @staticmethod
    def Enum(cls, name):
        """int32 in the serialized data. Numbers this build does not know stay plain ints."""
        scalar = _SCALARS[INT]

        def known(number):
            try:
                return cls(number)
            except ValueError:
                return number

        def write(out, value):
            out += _pack(scalar, int(value), "int")

        def read(data, offset, length, depth):
            _scalar_size(length, 4)
            return known(scalar.unpack_from(data, offset)[0])

        return Item(INT, cls.__name__, write, read, lambda b, v: b.enum_value(v, name), "i",
                    lambda value, key: int(value), known)

    @staticmethod
    def Message(cls):
        def write(out, value):
            if not isinstance(value, cls):
                raise ProtacoError("", "%r is not a %s" % (value, cls.__name__))
            _write_body(out, value)

        def read(data, offset, length, depth):
            message = cls(_stamp=NO_STAMP)
            _read_body(message, data, offset, length, depth + 1, False)
            return message

        return Item(MESSAGE, cls.__name__, write, read, lambda b, v: b.message(v))

    @staticmethod
    def List(element):
        def write(out, value):
            items = value if value is not None else ()
            out += _U8.pack(element.type)
            out += _U32.pack(len(items))
            for i, item in enumerate(items):
                at = _begin_sized(out)
                try:
                    element.write(out, item)
                except ProtacoError as e:
                    raise e._prepend("[%d]" % i) from None
                _end_sized(out, at)

        def read(data, offset, length, depth):
            p = offset
            end = offset + length
            if length < 5:
                raise ProtacoError("", "list header truncated")
            if data[p] != element.type:
                raise ProtacoError("", "list element type mismatch")
            count = _U32.unpack_from(data, p + 1)[0]
            p += 5
            if count > (end - p) // 4:
                raise ProtacoError("", "list count too large")
            items = []
            for i in range(count):
                size = _take_size(data, p, end)
                if size is None:
                    raise ProtacoError("[%d]" % i, "list element truncated")
                p += 4
                try:
                    items.append(element.read(data, p, size, depth))
                except ProtacoError as e:
                    raise e._prepend("[%d]" % i) from None
                p += size
            if p != end:
                raise ProtacoError("", "trailing bytes in list")
            return items

        return Item(LIST, "list", write, read, lambda b, v: b.list_items(v, element))

    @staticmethod
    def Map(key, value):
        # Entries in the dict's order: two maps with the same entries may differ in bytes.
        def write(out, entries):
            entries = entries if entries is not None else {}
            out += _U8.pack(key.type)
            out += _U8.pack(value.type)
            out += _U32.pack(len(entries))
            for i, (k, v) in enumerate(entries.items()):
                try:
                    at = _begin_sized(out)
                    key.write(out, k)
                    _end_sized(out, at)
                    at = _begin_sized(out)
                    value.write(out, v)
                    _end_sized(out, at)
                except ProtacoError as e:
                    raise e._prepend("[%d]" % i) from None

        def read(data, offset, length, depth):
            p = offset
            end = offset + length
            if length < 6:
                raise ProtacoError("", "map header truncated")
            if data[p] != key.type or data[p + 1] != value.type:
                raise ProtacoError("", "map key or value type mismatch")
            count = _U32.unpack_from(data, p + 2)[0]
            p += 6
            if count > (end - p) // 8:
                raise ProtacoError("", "map count too large")
            entries = {}
            for i in range(count):
                at = "[%d]" % i
                size = _take_size(data, p, end)
                if size is None:
                    raise ProtacoError(at, "map key truncated or bad")
                p += 4
                try:
                    k = key.read(data, p, size, depth)
                except ProtacoError:
                    raise ProtacoError(at, "map key truncated or bad") from None
                p += size
                size = _take_size(data, p, end)
                if size is None:
                    raise ProtacoError(at, "map value truncated")
                p += 4
                try:
                    v = value.read(data, p, size, depth)
                except ProtacoError as e:
                    raise e._prepend(at) from None
                p += size
                if k in entries:
                    raise ProtacoError(at, "duplicate map key")
                entries[k] = v
            if p != end:
                raise ProtacoError("", "trailing bytes in map")
            return entries

        return Item(MAP, "map", write, read, lambda b, v: b.map_items(v, key, value))


# ---- messages ----

class _Schema:
    __slots__ = ("name", "version", "min_version", "hashes", "allow_extension", "schema_keys", "fixed", "fixed_struct",
                 "fixed_size", "var", "text", "attrs")

    def __init__(self, cls, hashes, allow_extension, schema_keys, fixed, var, text):
        self.name = _utf8(cls.MsgName)
        self.version = cls.MsgVersion
        self.min_version = cls.MsgMinVersion
        self.hashes = hashes
        self.allow_extension = allow_extension
        self.schema_keys = frozenset(schema_keys)
        self.fixed = fixed   # (key, attr or None for a removed slot, item, default of a removed slot)
        self.fixed_struct = struct.Struct("<" + "".join(item.code for _, _, item, _ in fixed))
        self.fixed_size = self.fixed_struct.size
        self.var = tuple((_utf8(key), key, attr, item, required, optional) for key, attr, item, required, optional in var)
        items = {key: (attr, item) for key, attr, item, _ in fixed if attr is not None}
        items.update((key, (attr, item)) for key, attr, item, _, _ in var)
        self.text = tuple((key,) + items[key] for key in text)
        self.attrs = {key: attr for key, (attr, _) in items.items()}
        self.attrs.update((attr, attr) for attr, _ in items.values())

    def pack_fixed(self, message):
        values = []
        for key, attr, item, default in self.fixed:
            values.append(item.to_fixed(getattr(message, attr) if attr is not None else default, key))
        try:
            return self.fixed_struct.pack(*values)
        except (struct.error, TypeError, OverflowError):
            for (key, _, item, _), value in zip(self.fixed, values):
                try:
                    struct.pack("<" + item.code, value)
                except (struct.error, TypeError, OverflowError):
                    raise ProtacoError(key, "%r does not fit %s" % (value, item.name)) from None
            raise

    def unpack_fixed(self, message, data, offset):
        for (_, attr, item, _), value in zip(self.fixed, self.fixed_struct.unpack_from(data, offset)):
            if attr is not None:
                setattr(message, attr, item.from_fixed(value))


class Message:
    """Base of every generated message: the envelope carried in the frame header, and ext.
    A new object gets id = NextId() and ts = NowUs(); corr starts at 0. Objects created by the decoder or as nested
    fields (_stamp=NO_STAMP) get none. auto_envelope false in the yaml, or DisableAutoEnvelope(), leaves id and ts at 0."""

    __slots__ = ("_env", "_ext")

    def __init__(self, _stamp=None):
        self._ext = None
        if _stamp is NO_STAMP or not _auto_envelope:
            self._env = Envelope()
        else:
            self._env = Envelope(NextId(), 0, NowUs())

    @classmethod
    def _Bind(cls, hashes, allow_extension, schema_keys, fixed, var, text):
        cls._SCHEMA = _Schema(cls, hashes, allow_extension, schema_keys, fixed, var, text)

    def _Assign(self, fields):
        attrs = type(self)._SCHEMA.attrs
        for key, value in fields.items():
            attr = attrs.get(key)
            if attr is None:
                raise TypeError("%s has no field '%s'" % (type(self).MsgName, key))
            setattr(self, attr, value)

    @property
    def ext(self):
        """Extra key-value data not in the yaml. Unknown keys of received messages are kept here."""
        if self._ext is None:
            schema = type(self)._SCHEMA
            self._ext = Ext(schema.schema_keys, schema.allow_extension)
        return self._ext

    def Serialize(self):
        """Frame bytes, envelope included. Raises ProtacoError ("path: reason") for a value that does not fit its type."""
        schema = type(self)._SCHEMA
        out = bytearray(b"PC")
        out.append(FRAME_VERSION)
        out.append(len(schema.name))
        out += schema.name
        out += _U16.pack(schema.version)
        out += _U16.pack(schema.min_version)
        env = self._env
        try:
            out += _ENV.pack(env.id, env.corr, env.ts)
        except (struct.error, TypeError):
            raise ProtacoError("envelope", "%r does not fit id uint, corr uint, ts int64" % (env,)) from None
        body_at = _begin_sized(out)
        for schema_hash in schema.hashes:
            out += _U32.pack(schema_hash)
        start = len(out)
        _write_body(out, self)
        _U32.pack_into(out, body_at, len(out) - start)
        return bytes(out)

    def ToString(self, style=None):
        """Text for logs: field names and values, ext entries under ext{...}. TextStyle.Pretty() for one field per line."""
        builder = _TextBuilder(style)
        builder.message(self, True)
        return builder.result()

    def __str__(self):
        return self.ToString()

    def __repr__(self):
        return self.ToString()

    def GetEnvelope(self):
        env = self._env
        return Envelope(env.id, env.corr, env.ts)

    def SetEnvelope(self, env):
        self._env = Envelope(env.id, env.corr, env.ts)

    def GetId(self):
        return self._env.id

    def SetId(self, id):
        self._env.id = id

    def GetCorr(self):
        """Id of the message this one answers. Set by the user."""
        return self._env.corr

    def SetCorr(self, corr):
        self._env.corr = corr

    def GetTimestamp(self):
        """UTC microseconds since 1970-01-01."""
        return self._env.ts

    def SetTimestamp(self, timestamp_us):
        self._env.ts = timestamp_us

    def SetTimestampAsNow(self):
        self._env.ts = NowUs()

    def TryGetTimeFromTimestamp(self):
        """Timestamp as local "YYYY-MM-DD" and "hh:mm:ss.ffffff". Result value: (date, time)."""
        return TryGetTime(self._env.ts)

    def TryGetTimeFromTimestampParts(self):
        """Result value: (year, month, day, hour, minute, second, millisecond, microsecond)."""
        return TryGetTimeParts(self._env.ts)


def _write_tlv(out, key, kind, write, value):
    out += _U8.pack(len(key))
    out += key
    out += _U8.pack(kind)
    at = _begin_sized(out)
    write(out, value)
    _end_sized(out, at)


def _write_raw(out, value):
    out += value


# body: FixedLen u32 | fixed block | variable TLV... | ext TLV...
def _write_body(out, message):
    schema = type(message)._SCHEMA
    out += _U32.pack(schema.fixed_size)
    out += schema.pack_fixed(message)
    for key_bytes, key, attr, item, _, optional in schema.var:
        value = getattr(message, attr)
        if optional and value is None:
            continue
        try:
            _write_tlv(out, key_bytes, item.type, item.write, value)
        except ProtacoError as e:
            raise e._prepend(key) from None
    if message._ext is not None:
        for key, kind, value in message._ext._entries:
            _write_tlv(out, _utf8(key), kind, _write_raw, value)


class _VarReader:
    __slots__ = ("fields",)

    def __init__(self, data, offset, count):
        self.fields = {}   # key -> [type, offset, length, used]
        p = offset
        end = offset + count
        while p < end:
            key_length = data[p]
            if key_length == 0 or p + 1 + key_length > end:
                raise ProtacoError("", "field key truncated")
            key = _text(data, p + 1, key_length)
            p += 1 + key_length
            if p + 5 > end:
                raise ProtacoError(key, "field value truncated")
            kind = data[p]
            length = _U32.unpack_from(data, p + 1)[0]
            p += 5
            if length > end - p:
                raise ProtacoError(key, "field value truncated")
            if key in self.fields:
                raise ProtacoError(key, "duplicate key")
            if len(self.fields) >= MAX_FIELDS:
                raise ProtacoError("", "too many fields")
            self.fields[key] = [kind, p, length, False]
            p += length

    def take(self, key, kind, required):
        raw = self.fields.get(key)
        if raw is None:
            if required:
                raise ProtacoError(key, "missing required")
            return None
        raw[3] = True
        if raw[0] != kind:
            raise ProtacoError(key, "type mismatch: expected %s, got %s" % (_type_name(kind), _type_name(raw[0])))
        return raw

    # Keys not in the schema are kept in ext. allow_extension false rejects them, except fields of a newer version,
    # which are dropped.
    def move_unused(self, message, data, allow_extension, newer):
        for key, (kind, offset, length, used) in self.fields.items():
            if used:
                continue
            if not allow_extension:
                if newer:
                    continue
                raise ProtacoError(key, "unknown key (allow_extension false)")
            message.ext._entries.append([key, kind, bytes(data[offset:offset + length])])


# FixedLen u32 | fixed block. Same version means same size; a newer version only appends. Also guards nested bodies,
# which carry no fingerprint of their own.
def _take_fixed(data, offset, count, expected, newer):
    if count < 4:
        raise ProtacoError("", "fixed block truncated")
    fixed_length = _U32.unpack_from(data, offset)[0]
    if fixed_length > count - 4:
        raise ProtacoError("", "fixed block truncated")
    if fixed_length < expected if newer else fixed_length != expected:
        raise ProtacoError("", "fixed block size mismatch: expected %d, got %d" % (expected, fixed_length))
    return fixed_length


# newer: the frame is of a newer version than message (verified by fingerprint). message reads its own prefix of it.
def _read_body(message, data, offset, count, depth, newer):
    if depth > MAX_DEPTH:
        raise ProtacoError("", "nesting too deep")
    schema = type(message)._SCHEMA
    fixed_length = _take_fixed(data, offset, count, schema.fixed_size, newer)
    if schema.fixed_size:
        schema.unpack_fixed(message, data, offset + 4)
    var_start = offset + 4 + fixed_length
    reader = _VarReader(data, var_start, offset + count - var_start)
    for _, key, attr, item, required, _ in schema.var:
        raw = reader.take(key, item.type, required)
        if raw is None:
            continue
        try:
            setattr(message, attr, item.read(data, raw[1], raw[2], depth))
        except ProtacoError as e:
            raise e._prepend(key) from None
    reader.move_unused(message, data, schema.allow_extension, newer)


def _read_header(data):
    if len(data) < 28:
        raise ProtacoError("", "frame too short")
    if data[0] != 0x50 or data[1] != 0x43:
        raise ProtacoError("", "bad magic")
    if data[2] != FRAME_VERSION:
        raise ProtacoError("", "unknown frame version %d, expected %d" % (data[2], FRAME_VERSION))
    name_length = data[3]
    if len(data) < 28 + name_length:
        raise ProtacoError("", "frame too short")
    p = 4 + name_length
    version, min_version = struct.unpack_from("<HH", data, p)
    if min_version == 0 or min_version > version or version - min_version >= MAX_SCHEMA_VERSIONS:
        raise ProtacoError("", "bad version range %d..%d" % (min_version, version))
    hash_count = version - min_version + 1
    if len(data) < 28 + name_length + 4 * hash_count:
        raise ProtacoError("", "frame too short")
    header = Header()
    header.name = _text(data, 4, name_length)
    header.version = version
    header.minVersion = min_version
    header.env = Envelope(*_ENV.unpack_from(data, p + 4))
    header.bodySize = _U32.unpack_from(data, p + 20)[0]
    header.schemaHash = _U32.unpack_from(data, p + 24 + 4 * (hash_count - 1))[0]
    return header


# Checks shared by ToObject: header, name, supported versions, own fingerprint, body length.
def _check_frame(data, cls):
    if len(data) > MAX_FRAME_BYTES:
        raise ProtacoError("", "frame too large")
    header = _read_header(data)
    name = cls.MsgName
    mine = cls.MsgVersion
    if header.name != name:
        raise ProtacoError("", "message name mismatch: expected %s, got %s" % (name, header.name))
    if header.version < mine:
        what = "message version too old: %s reads version %d or newer, got %d" % (name, mine, header.version)
        if header.version >= cls.MsgMinVersion:
            what += ". Use the v%d type" % header.version
        raise ProtacoError("", what)
    if header.minVersion > mine:
        raise ProtacoError("", "message version too new: %s reads version %d, sender supports %d..%d"
                           % (name, mine, header.minVersion, header.version))
    name_length = data[3]
    schema_hash = _U32.unpack_from(data, 28 + name_length + 4 * (mine - header.minVersion))[0]
    if schema_hash != cls.MsgSchemaHash:
        raise ProtacoError("", "schema mismatch: %s version %d expected 0x%08X, got 0x%08X" % (name, mine, cls.MsgSchemaHash, schema_hash))
    header_size = 28 + name_length + 4 * (header.version - header.minVersion + 1)
    left = len(data) - header_size
    if left != header.bodySize:
        raise ProtacoError("", "frame truncated" if left < header.bodySize else "trailing bytes after frame")
    return header, header_size


def ToObject(cls, data):
    """Bytes to object, envelope included. Raises ProtacoError on failure.
    Accepts frames of cls's version and newer: a newer frame is read as cls's version (README.md "Versioning")."""
    if not isinstance(data, (bytes, bytearray)):
        data = bytes(data)
    header, header_size = _check_frame(data, cls)
    message = cls(_stamp=NO_STAMP)
    message._env = header.env
    newer = header.version > cls.MsgVersion
    try:
        _read_body(message, data, header_size, header.bodySize, 0, newer)
    except ProtacoError as e:
        if newer:
            raise ProtacoError(e.path, e.what + " (frame version %d read as %d)" % (header.version, cls.MsgVersion)) from None
        raise
    return message


def TryToObject(cls, data):
    """Bytes to object without exceptions. Result value: the message; error: "path: reason"."""
    try:
        return Result(True, ToObject(cls, data))
    except ProtacoError as e:
        return Result(False, None, str(e))


def PeekHeader(data):
    """Frame header only, without the body (for dispatch). Result value: Header."""
    try:
        if not isinstance(data, (bytes, bytearray)):
            data = bytes(data)
        return Result(True, _read_header(data))
    except ProtacoError as e:
        return Result(False, None, str(e))


# ---- text ----

class _TextBuilder:
    """Text for ToString. Same text as C++ for ASCII values (strings are cut by characters, as in C#)."""

    __slots__ = ("style", "out", "first")

    def __init__(self, style):
        self.style = style if style is not None else TextStyle()
        self.out = []
        self.first = []   # per open level: no item written yet

    def result(self):
        return "".join(self.out)

    # Name{field=value, ...}. The top-level message shows its envelope first when TextStyle.envelope is set.
    def message(self, message, top=False):
        if message is None:
            self.out.append("null")
            return
        schema = type(message)._SCHEMA
        self.out.append(type(message).MsgName)
        self.open(self.style.open)
        if top and self.style.envelope:
            self.envelope_item(message._env)
        for key, attr, item in schema.text:
            self.key(key)
            item.text(self, getattr(message, attr))
        ext = message._ext
        if ext is not None and ext._entries:
            self.item()
            self.out.append("ext")
            self.open(self.style.open)
            for key, kind, value in ext._entries:
                self.key(key)
                self.raw_value(kind, value)
            self.close(self.style.close)
        self.close(self.style.close)

    # envelope{id=7, corr=0, ts=2026-10-20 14:03:22.123456}. Ts outside the time range prints as the integer.
    def envelope_item(self, env):
        self.item()
        time_text = TryGetTime(env.ts)
        ts = time_text.value[0] + " " + time_text.value[1] if time_text else "%d" % env.ts
        style = self.style
        self.out.append("envelope%sid=%d%scorr=%d%sts=%s%s" % (style.open, env.id, style.separator, env.corr, style.separator, ts, style.close))

    def key(self, key):
        self.item()
        self.out.append(key)
        self.out.append("=")

    def list_items(self, items, element):
        self.open("[")
        for value in items if items is not None else ():
            self.item()
            element.text(self, value)
        self.close("]")

    # {key: value, ...} with keys sorted, so the text does not depend on the dict order.
    def map_items(self, entries, key, value):
        self.open("{")
        entries = entries if entries is not None else {}
        for k in sorted(entries):
            self.item()
            key.text(self, k)
            self.out.append(": ")
            value.text(self, entries[k])
        self.close("}")

    def bool_value(self, value):
        self.out.append("true" if value else "false")

    def int_value(self, value):
        self.out.append("%d" % value)

    # Same digits as C++ "%.15g".
    def double_value(self, value):
        if math.isnan(value):
            self.out.append("nan")
        elif math.isinf(value):
            self.out.append("-inf" if value < 0 else "inf")
        elif value == 0:
            self.out.append("0")   # -0 too
        else:
            self.out.append("%.15g" % value)

    def string_value(self, value):
        value = value if value is not None else ""
        shown = min(len(value), self.style.maxText)
        self.out.append('"')
        self.out.append("".join(_escaped(c) for c in value[:shown]))
        self.out.append('"')
        if shown < len(value):
            self.out.append("...(+%d)" % (len(value) - shown))

    def bytes_value(self, value):
        value = bytes(value) if value is not None else b""
        shown = min(len(value), min(self.style.maxText, 8))
        text = "<%d bytes" % len(value)
        for i in range(shown):
            text += (": " if i == 0 else " ") + "%02X" % value[i]
        if shown < len(value):
            text += " ..."
        self.out.append(text + ">")

    # Enum: Name(number), ?(number) for a number this build does not know.
    def enum_value(self, value, name):
        text = name(value)
        self.out.append("%s(%d)" % (text if text is not None else "?", int(value)))

    # ext values carry only their kind, not a schema.
    def raw_value(self, kind, value):
        scalar = _SCALARS.get(kind)
        if scalar is not None:
            if len(value) != scalar.size:
                self.out.append("<bad value>")
            elif kind == BOOL:
                self.bool_value(value[0] != 0)
            elif kind == DOUBLE:
                self.double_value(scalar.unpack(value)[0])
            else:
                self.int_value(scalar.unpack(value)[0])
        elif kind == STRING:
            self.string_value(value.decode("utf-8", "replace"))
        elif kind == BYTES:
            self.bytes_value(value)
        elif kind == MESSAGE:
            self.out.append("<message %d bytes>" % len(value))
        elif kind == LIST:
            self.out.append("<list %d bytes>" % len(value))
        elif kind == MAP:
            self.out.append("<map %d bytes>" % len(value))
        else:
            self.out.append("<type %d, %d bytes>" % (kind, len(value)))

    def open(self, text):
        self.out.append(text)
        self.first.append(True)

    def close(self, text):
        empty = self.first.pop()
        if self.style.indent > 0 and not empty:
            self.new_line()
        self.out.append(text)

    def item(self):
        if not self.first[-1]:
            self.out.append("," if self.style.indent > 0 else self.style.separator)
        self.first[-1] = False
        if self.style.indent > 0:
            self.new_line()

    def new_line(self):
        self.out.append("\n" + " " * (len(self.first) * self.style.indent))


_ESCAPES = {'"': '\\"', "\\": "\\\\", "\n": "\\n", "\r": "\\r", "\t": "\\t"}


def _escaped(c):
    escaped = _ESCAPES.get(c)
    if escaped is not None:
        return escaped
    code = ord(c)
    if code < 0x20 or code == 0x7F:
        return "\\x%02X" % code
    return c
