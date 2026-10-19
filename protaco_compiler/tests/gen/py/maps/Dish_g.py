# Dish_g.py. protacoc 0.6.0, source Dish.yaml (sha1 7f274cbd1668). Do not edit.
import protaco


class Dish(protaco.Message):
    """One dish.
    [version 1; schema 0xE2531233; source Dish.yaml]
    """

    __slots__ = ("name", "price")
    MsgName = "Dish"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xE2531233

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Dish(name=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string]
        self.name = ""
        #: [type double]
        self.price = 0.0
        if fields:
            self._Assign(fields)


Dish._Bind(
    hashes=(0xE2531233,),
    allow_extension=True,
    schema_keys=("name", "price"),
    fixed=(
        ("price", "price", protaco.Items.Double, None),
    ),
    var=(
        ("name", "name", protaco.Items.String, False, False),
    ),
    text=("name", "price"),
)
