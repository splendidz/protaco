# Trail_g.py. protacoc 0.7.0, source Trail.yaml (sha1 4eba086e08ee). Do not edit.
import protaco
from .Point_g import Point


class Trail(protaco.Message):
    """A fixed message in a list stays a nested body; inline next to variable fields.
    [version 1; schema 0x13BA591D; source Trail.yaml]
    """

    __slots__ = ("start", "points", "note")
    MsgName = "Trail"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x13BA591D

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Trail(start=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type Point; fixed message, inline 8 bytes]
        self.start = Point(_stamp=protaco.NO_STAMP)
        #: [type list<Point>]
        self.points = []
        #: [type string]
        self.note = ""
        if fields:
            self._Assign(fields)


Trail._Bind(
    hashes=(0x13BA591D,),
    allow_extension=True,
    schema_keys=("start", "points", "note"),
    fixed=(
        ("start", "start", protaco.Items.Fixed(Point), None),
    ),
    var=(
        ("points", "points", protaco.Items.List(protaco.Items.Message(Point)), False, False),
        ("note", "note", protaco.Items.String, False, False),
    ),
    text=("start", "points", "note"),
)
