# Pie_g.py. protacoc 0.7.0, source Pie.yaml (sha1 b9d29a89197b). Do not edit.
import protaco
from .PizzaSize_enum import PizzaSize, PizzaSizeNames


class Pie(protaco.Message):
    """A pie with enum fields.
    [version 1; schema 0xB67183D5; source Pie.yaml]
    """

    __slots__ = ("size", "crust", "sizes", "name")
    MsgName = "Pie"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xB67183D5

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Pie(size=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type PizzaSize; default Large]
        self.size = PizzaSize.Large
        #: No default: the first value of the enum (Small).
        #: [type PizzaSize]
        self.crust = PizzaSize.Small
        #: [type list<PizzaSize>]
        self.sizes = []
        #: [type string; default ""]
        self.name = ""
        if fields:
            self._Assign(fields)


Pie._Bind(
    hashes=(0xB67183D5,),
    allow_extension=True,
    schema_keys=("size", "crust", "sizes", "name"),
    fixed=(
        ("size", "size", protaco.Items.Enum(PizzaSize, PizzaSizeNames.Name), None),
        ("crust", "crust", protaco.Items.Enum(PizzaSize, PizzaSizeNames.Name), None),
    ),
    var=(
        ("sizes", "sizes", protaco.Items.List(protaco.Items.Enum(PizzaSize, PizzaSizeNames.Name)), False, False),
        ("name", "name", protaco.Items.String, False, False),
    ),
    text=("size", "crust", "sizes", "name"),
)
