# PizzaSize_enum.py. protacoc 0.7.1, source PizzaSize.enum.yaml (sha1 1b92f7597053). Do not edit.
import enum

import protaco


class PizzaSize(enum.IntEnum):
    """Pizza size. Values are int32 in the serialized data; ToString prints Large(3).
    [enum; version 1; source PizzaSize.enum.yaml]
    """

    #: 10 inch
    Small = 1
    #: 12 inch
    Medium = 2
    #: 14 inch
    Large = 3


class PizzaSizeNames:
    """Names of PizzaSize values."""

    _NAMES = {1: "Small", 2: "Medium", 3: "Large"}   # removed values included
    _VALUES = {"Small": PizzaSize.Small, "Medium": PizzaSize.Medium, "Large": PizzaSize.Large}

    @staticmethod
    def Name(value):
        """Name of a value, removed values included. None for a number never declared."""
        return PizzaSizeNames._NAMES.get(int(value))

    @staticmethod
    def TryParse(name):
        """Value from its yaml name (values of version 1). Result value: the PizzaSize."""
        value = PizzaSizeNames._VALUES.get(name)
        if value is None:
            return protaco.Result(False, None, "unknown PizzaSize name: " + str(name))
        return protaco.Result(True, value)
