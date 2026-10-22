# Topping_g.py. protacoc 0.7.1, source Topping.yaml (sha1 f343ff787f9f). Do not edit.
import protaco


class Topping(protaco.Message):
    """One topping. Element of PizzaOrder.toppings.
    [version 1; schema 0xEB8A11D8; source Topping.yaml]
    """

    __slots__ = ("name", "grams")
    MsgName = "Topping"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xEB8A11D8

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Topping(name=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: topping name
        #: [type string(24); fixed 24 bytes, set rejects longer values]
        self.name = ""
        #: weight in grams
        #: [type int; default 30]
        self.grams = 30
        if fields:
            self._Assign(fields)


Topping._Bind(
    hashes=(0xEB8A11D8,),
    allow_extension=True,
    schema_keys=("name", "grams"),
    fixed=(
        ("name", "name", protaco.Items.Chars(24), None),
        ("grams", "grams", protaco.Items.Int, None),
    ),
    var=(),
    text=("name", "grams"),
)
