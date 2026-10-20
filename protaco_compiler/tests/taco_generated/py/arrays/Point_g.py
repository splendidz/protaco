# Point_g.py. protacoc 0.6.0, source Point.yaml (sha1 8bf074d3ebbb). Do not edit.
import protaco


class Point(protaco.Message):
    """A 2D point. fixed: true, so it sits inline in the fixed block of other messages.
    [version 1; schema 0x53239A70; fixed: inline in other messages; source Point.yaml]
    """

    __slots__ = ("x", "y")
    MsgName = "Point"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x53239A70

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Point(x=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type int; default -1]
        self.x = -1
        #: [type int]
        self.y = 0
        if fields:
            self._Assign(fields)


Point._Bind(
    hashes=(0x53239A70,),
    allow_extension=False,
    schema_keys=("x", "y"),
    fixed=(
        ("x", "x", protaco.Items.Int, None),
        ("y", "y", protaco.Items.Int, None),
    ),
    var=(),
    text=("x", "y"),
)
