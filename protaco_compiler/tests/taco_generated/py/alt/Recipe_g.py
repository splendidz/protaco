# Recipe_g.py. protacoc 0.7.0, source Recipe.yaml (sha1 841cd482eb1d). Do not edit.
import protaco


class Recipe(protaco.Message):
    """Fingerprint test. Same name, version and fixed block size as ver_v1/Recipe.yaml, fixed fields in another order.
    Without the fingerprint, name and step would be read from each other's bytes without an error.
    [version 1; schema 0xD1D83D6C; source Recipe.yaml]
    """

    __slots__ = ("step", "name", "path")
    MsgName = "Recipe"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xD1D83D6C

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Recipe(step=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type int; default 3]
        self.step = 3
        #: recipe name
        #: [type string(16); fixed 16 bytes, set rejects longer values]
        self.name = ""
        #: [type string; required, decoding fails if missing]
        self.path = ""
        if fields:
            self._Assign(fields)


Recipe._Bind(
    hashes=(0xD1D83D6C,),
    allow_extension=True,
    schema_keys=("step", "name", "path"),
    fixed=(
        ("step", "step", protaco.Items.Int, None),
        ("name", "name", protaco.Items.Chars(16), None),
    ),
    var=(
        ("path", "path", protaco.Items.String, True, False),
    ),
    text=("step", "name", "path"),
)
