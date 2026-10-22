# PizzaSize_enum.py. protacoc 0.7.1, source PizzaSize.enum.yaml (sha1 ba3536e7746d). Do not edit.
import enum

import protaco


class PizzaSize(enum.IntEnum):
    """Pizza size. Kids was dropped in version 3; its number stays reserved and its name still prints.
    [enum; version 3; source PizzaSize.enum.yaml]
    """

    #: 10 inch
    Small = 1
    Medium = 2
    Large = 3
    #: [added in 2]
    Family = 4


class PizzaSizeNames:
    """Names of PizzaSize values."""

    _NAMES = {1: "Small", 2: "Medium", 3: "Large", 5: "Kids", 4: "Family"}   # removed values included
    _VALUES = {"Small": PizzaSize.Small, "Medium": PizzaSize.Medium, "Large": PizzaSize.Large, "Family": PizzaSize.Family}

    @staticmethod
    def Name(value):
        """Name of a value, removed values included. None for a number never declared."""
        return PizzaSizeNames._NAMES.get(int(value))

    @staticmethod
    def TryParse(name):
        """Value from its yaml name (values of version 3). Result value: the PizzaSize."""
        value = PizzaSizeNames._VALUES.get(name)
        if value is None:
            return protaco.Result(False, None, "unknown PizzaSize name: " + str(name))
        return protaco.Result(True, value)
