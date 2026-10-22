# Recipe_g.py. protacoc 0.7.1, source Recipe.yaml (sha1 a8ae1c9a0807). Do not edit.
import protaco


class Recipe(protaco.Message):
    """Version test. Pairs with ver_v2 (other version) and alt (same version, other fields).
    [version 1; schema 0x92D7F812; source Recipe.yaml]
    """

    __slots__ = ("name", "step", "path")
    MsgName = "Recipe"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x92D7F812

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Recipe(name=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: recipe name
        #: [type string(16); fixed 16 bytes, set rejects longer values]
        self.name = ""
        #: [type int; default 3]
        self.step = 3
        #: [type string; required, decoding fails if missing]
        self.path = ""
        if fields:
            self._Assign(fields)


Recipe._Bind(
    hashes=(0x92D7F812,),
    allow_extension=True,
    schema_keys=("name", "step", "path"),
    fixed=(
        ("name", "name", protaco.Items.Chars(16), None),
        ("step", "step", protaco.Items.Int, None),
    ),
    var=(
        ("path", "path", protaco.Items.String, True, False),
    ),
    text=("name", "step", "path"),
)
