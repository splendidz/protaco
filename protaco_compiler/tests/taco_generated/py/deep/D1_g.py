# D1_g.py. protacoc 0.7.1, source D1.yaml (sha1 3d397ff46a35). Do not edit.
import protaco
from .D0_g import D0


class D1(protaco.Message):
    """[version 1; schema 0x45526499; source D1.yaml]
    """

    __slots__ = ("d",)
    MsgName = "D1"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x45526499

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D1(d=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type D0]
        self.d = D0(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


D1._Bind(
    hashes=(0x45526499,),
    allow_extension=True,
    schema_keys=("d",),
    fixed=(),
    var=(
        ("d", "d", protaco.Items.Message(D0), False, False),
    ),
    text=("d",),
)
