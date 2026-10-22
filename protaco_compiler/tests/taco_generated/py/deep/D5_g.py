# D5_g.py. protacoc 0.7.1, source D5.yaml (sha1 ba4fb7e5a44a). Do not edit.
import protaco
from .D4_g import D4


class D5(protaco.Message):
    """[version 1; schema 0x66677892; source D5.yaml]
    """

    __slots__ = ("d",)
    MsgName = "D5"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x66677892

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D5(d=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type D4]
        self.d = D4(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


D5._Bind(
    hashes=(0x66677892,),
    allow_extension=True,
    schema_keys=("d",),
    fixed=(),
    var=(
        ("d", "d", protaco.Items.Message(D4), False, False),
    ),
    text=("d",),
)
