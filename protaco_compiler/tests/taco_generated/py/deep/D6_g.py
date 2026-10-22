# D6_g.py. protacoc 0.7.1, source D6.yaml (sha1 f9f37dcf3425). Do not edit.
import protaco
from .D5_g import D5


class D6(protaco.Message):
    """[version 1; schema 0xC7D65C36; source D6.yaml]
    """

    __slots__ = ("d",)
    MsgName = "D6"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xC7D65C36

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D6(d=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type D5]
        self.d = D5(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


D6._Bind(
    hashes=(0xC7D65C36,),
    allow_extension=True,
    schema_keys=("d",),
    fixed=(),
    var=(
        ("d", "d", protaco.Items.Message(D5), False, False),
    ),
    text=("d",),
)
