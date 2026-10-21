# Slot_g.py. protacoc 0.7.0, source Slot.yaml (sha1 c2aa1108a1e1). Do not edit.
import protaco
from .Level_enum import Level, LevelNames
from .Point_g import Point


class Slot(protaco.Message):
    """One slot of a rack: a fixed message holding a fixed message, an array of them and scalar arrays.
    [version 1; schema 0x9D40CDF3; fixed: inline in other messages; source Slot.yaml]
    """

    __slots__ = ("id", "name", "pos", "path", "level", "temps")
    MsgName = "Slot"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x9D40CDF3

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Slot(id=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type uint]
        self.id = 0
        #: [type string(8); fixed 8 bytes, set rejects longer values; default "empty"]
        self.name = "empty"
        #: [type Point; fixed message, inline 8 bytes]
        self.pos = Point(_stamp=protaco.NO_STAMP)
        #: [type Point[2]; 2 elements, fixed 16 bytes]
        self.path = [Point(_stamp=protaco.NO_STAMP) for _ in range(2)]
        #: [type Level]
        self.level = Level.Low
        #: [type double[2]; 2 elements, fixed 16 bytes]
        self.temps = [0.0] * 2
        if fields:
            self._Assign(fields)


Slot._Bind(
    hashes=(0x9D40CDF3,),
    allow_extension=False,
    schema_keys=("id", "name", "pos", "path", "level", "temps"),
    fixed=(
        ("id", "id", protaco.Items.UInt, None),
        ("name", "name", protaco.Items.Chars(8), None),
        ("pos", "pos", protaco.Items.Fixed(Point), None),
        ("path", "path", protaco.Items.Array(protaco.Items.Fixed(Point), 2), None),
        ("level", "level", protaco.Items.Enum(Level, LevelNames.Name), None),
        ("temps", "temps", protaco.Items.Array(protaco.Items.Double, 2), None),
    ),
    var=(),
    text=("id", "name", "pos", "path", "level", "temps"),
)
