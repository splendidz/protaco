# Menu_g.py. protacoc 0.6.0, source Menu.yaml (sha1 886d288a340b). Do not edit.
import protaco
from .PizzaSize_enum import PizzaSize, PizzaSizeNames
from .Category_g import Category


class Menu(protaco.Message):
    """A store menu: an enum field, a map and a tree of sections.
    [version 1; schema 0x1A3924BC; source Menu.yaml]
    """

    __slots__ = ("store", "defaultSize", "prices", "sections")
    MsgName = "Menu"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x1A3924BC

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Menu(store=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: store code
        #: [type string(16); fixed 16 bytes, set rejects longer values]
        self.store = ""
        #: size when an order does not say
        #: [type PizzaSize; default Large]
        self.defaultSize = PizzaSize.Large
        #: Map types contain ',', so they are quoted inside {...}.
        #: price by dish name
        #: [type map<string, double>]
        self.prices = {}
        #: top-level sections
        #: [type list<Category>]
        self.sections = []
        if fields:
            self._Assign(fields)


Menu._Bind(
    hashes=(0x1A3924BC,),
    allow_extension=True,
    schema_keys=("store", "defaultSize", "prices", "sections"),
    fixed=(
        ("store", "store", protaco.Items.Chars(16), None),
        ("defaultSize", "defaultSize", protaco.Items.Enum(PizzaSize, PizzaSizeNames.Name), None),
    ),
    var=(
        ("prices", "prices", protaco.Items.Map(protaco.Items.String, protaco.Items.Double), False, False),
        ("sections", "sections", protaco.Items.List(protaco.Items.Message(Category)), False, False),
    ),
    text=("store", "defaultSize", "prices", "sections"),
)
