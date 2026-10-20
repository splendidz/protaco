# Rack_g.py. protacoc 0.6.0, source Rack.yaml (sha1 fd710f61bd22). Do not edit.
import protaco
from .Point_g import Point
from .Slot_g import Slot


class Rack(protaco.Message):
    """A rack of slots. Not fixed: true itself, but every field is fixed, so TryView reads it in place.
    [version 1; schema 0xD3E62388; allow_extension false; source Rack.yaml]
    """

    __slots__ = ("origin", "slots", "count")
    MsgName = "Rack"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xD3E62388

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Rack(origin=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type Point; fixed message, inline 8 bytes]
        self.origin = Point(_stamp=protaco.NO_STAMP)
        #: [type Slot[Sizes.Slots]; 4 elements, fixed 224 bytes]
        self.slots = [Slot(_stamp=protaco.NO_STAMP) for _ in range(4)]
        #: [type int]
        self.count = 0
        if fields:
            self._Assign(fields)


Rack._Bind(
    hashes=(0xD3E62388,),
    allow_extension=False,
    schema_keys=("origin", "slots", "count"),
    fixed=(
        ("origin", "origin", protaco.Items.Fixed(Point), None),
        ("slots", "slots", protaco.Items.Array(protaco.Items.Fixed(Slot), 4), None),
        ("count", "count", protaco.Items.Int, None),
    ),
    var=(),
    text=("origin", "slots", "count"),
)
