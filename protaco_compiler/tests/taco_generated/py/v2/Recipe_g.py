# Recipe_g.py. protacoc 0.7.1, source Recipe.yaml (sha1 f97dfa2fc24c). Do not edit.
import protaco


class Recipe(protaco.Message):
    """Version test. Version 2 of ver_v1/Recipe.yaml with two more fields.
    [version 2; schema 0xAC2B89A3; source Recipe.yaml]
    """

    __slots__ = ("name", "temp", "step", "path", "note")
    MsgName = "Recipe"
    MsgVersion = 2
    MsgMinVersion = 2
    MsgSchemaHash = 0xAC2B89A3

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Recipe(name=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: recipe name
        #: [type string(16); fixed 16 bytes, set rejects longer values]
        self.name = ""
        #: temperature
        #: [type double; default 25.5]
        self.temp = 25.5
        #: [type int; default 3]
        self.step = 3
        #: [type string; required, decoding fails if missing]
        self.path = ""
        #: [type string; default "none"]
        self.note = "none"
        if fields:
            self._Assign(fields)


Recipe._Bind(
    hashes=(0xAC2B89A3,),
    allow_extension=True,
    schema_keys=("name", "temp", "step", "path", "note"),
    fixed=(
        ("name", "name", protaco.Items.Chars(16), None),
        ("temp", "temp", protaco.Items.Double, None),
        ("step", "step", protaco.Items.Int, None),
    ),
    var=(
        ("path", "path", protaco.Items.String, True, False),
        ("note", "note", protaco.Items.String, False, False),
    ),
    text=("name", "temp", "step", "path", "note"),
)
