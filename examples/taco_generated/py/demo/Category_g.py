# Category_g.py. protacoc 0.7.1, source Category.yaml (sha1 9b036f2023ea). Do not edit.
import protaco


class Category(protaco.Message):
    """A menu section. Sections nest: a section holds its subsections, so the message refers to itself.
    [version 1; schema 0x6B7C6E97; source Category.yaml]
    """

    __slots__ = ("name", "dishes", "subsections")
    MsgName = "Category"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x6B7C6E97

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Category(name=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: section name
        #: [type string]
        self.name = ""
        #: dish names
        #: [type list<string>]
        self.dishes = []
        #: sections inside this one
        #: [type list<Category>]
        self.subsections = []
        if fields:
            self._Assign(fields)


Category._Bind(
    hashes=(0x6B7C6E97,),
    allow_extension=True,
    schema_keys=("name", "dishes", "subsections"),
    fixed=(),
    var=(
        ("name", "name", protaco.Items.String, False, False),
        ("dishes", "dishes", protaco.Items.List(protaco.Items.String), False, False),
        ("subsections", "subsections", protaco.Items.List(protaco.Items.Message(Category)), False, False),
    ),
    text=("name", "dishes", "subsections"),
)
