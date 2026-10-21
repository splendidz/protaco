# Move_g.py. protacoc 0.7.0, source Move.yaml (sha1 a06bba8225f5). Do not edit.
import protaco


class Move(protaco.Message):
    """Keys that are Python keywords: attributes from_ and pass_ in Python, the yaml key everywhere else.
    [version 1; schema 0x97FD0219; source Move.yaml]
    """

    __slots__ = ("from_", "to", "pass_")
    MsgName = "Move"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x97FD0219

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Move(from_=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string(8); fixed 8 bytes, set rejects longer values]
        self.from_ = ""
        #: [type string(8); fixed 8 bytes, set rejects longer values]
        self.to = ""
        #: [type bool]
        self.pass_ = False
        if fields:
            self._Assign(fields)


Move._Bind(
    hashes=(0x97FD0219,),
    allow_extension=True,
    schema_keys=("from", "to", "pass"),
    fixed=(
        ("from", "from_", protaco.Items.Chars(8), None),
        ("to", "to", protaco.Items.Chars(8), None),
        ("pass", "pass_", protaco.Items.Bool, None),
    ),
    var=(),
    text=("from", "to", "pass"),
)
