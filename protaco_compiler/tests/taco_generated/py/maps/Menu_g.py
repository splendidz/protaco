# Menu_g.py. protacoc 0.7.0, source Menu.yaml (sha1 95302b23b98e). Do not edit.
import protaco
from .Grade_enum import Grade, GradeNames
from .Dish_g import Dish


class Menu(protaco.Message):
    """Every map key kind and value kind. Map types contain ',', so they are quoted inside {...}.
    [version 1; schema 0x0EC85362; source Menu.yaml]
    """

    __slots__ = ("prices", "stock", "specials", "notes", "vegan", "photos", "grades", "tags", "daily", "title")
    MsgName = "Menu"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x0EC85362

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Menu(prices=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type map<string, double>]
        self.prices = {}
        #: [type map<int, uint>; required, decoding fails if missing]
        self.stock = {}
        #: [type map<Grade, Dish>]
        self.specials = {}
        #: [type map<uint64, string>]
        self.notes = {}
        #: [type map<string, bool>]
        self.vegan = {}
        #: [type map<string, bytes>]
        self.photos = {}
        #: [type map<string, Grade>]
        self.grades = {}
        #: [type map<string, list<string>>]
        self.tags = {}
        #: [type map<int64, map<string, int>>]
        self.daily = {}
        #: [type string]
        self.title = ""
        if fields:
            self._Assign(fields)


Menu._Bind(
    hashes=(0x0EC85362,),
    allow_extension=True,
    schema_keys=("prices", "stock", "specials", "notes", "vegan", "photos", "grades", "tags", "daily", "title"),
    fixed=(),
    var=(
        ("prices", "prices", protaco.Items.Map(protaco.Items.String, protaco.Items.Double), False, False),
        ("stock", "stock", protaco.Items.Map(protaco.Items.Int, protaco.Items.UInt), True, False),
        ("specials", "specials", protaco.Items.Map(protaco.Items.Enum(Grade, GradeNames.Name), protaco.Items.Message(Dish)), False, False),
        ("notes", "notes", protaco.Items.Map(protaco.Items.UInt64, protaco.Items.String), False, False),
        ("vegan", "vegan", protaco.Items.Map(protaco.Items.String, protaco.Items.Bool), False, False),
        ("photos", "photos", protaco.Items.Map(protaco.Items.String, protaco.Items.Bytes), False, False),
        ("grades", "grades", protaco.Items.Map(protaco.Items.String, protaco.Items.Enum(Grade, GradeNames.Name)), False, False),
        ("tags", "tags", protaco.Items.Map(protaco.Items.String, protaco.Items.List(protaco.Items.String)), False, False),
        ("daily", "daily", protaco.Items.Map(protaco.Items.Int64, protaco.Items.Map(protaco.Items.String, protaco.Items.Int)), False, False),
        ("title", "title", protaco.Items.String, False, False),
    ),
    text=("prices", "stock", "specials", "notes", "vegan", "photos", "grades", "tags", "daily", "title"),
)
