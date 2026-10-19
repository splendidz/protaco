# D2_g.py. protacoc 0.6.0, source D2.yaml (sha1 1f32606f10c2). Do not edit.
import protaco
from .D1_g import D1


class D2(protaco.Message):
    """[version 1; schema 0x83009743; source D2.yaml]
    """

    __slots__ = ("d",)
    MsgName = "D2"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x83009743

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D2(d=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type D1]
        self.d = D1(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


D2._Bind(
    hashes=(0x83009743,),
    allow_extension=True,
    schema_keys=("d",),
    fixed=(),
    var=(
        ("d", "d", protaco.Items.Message(D1), False, False),
    ),
    text=("d",),
)
