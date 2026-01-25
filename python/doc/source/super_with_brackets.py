# ruff: noqa: D100, D101
from dataclasses import dataclass, field
from sphinxcontrib.bibtex.style.referencing import BracketStyle, PersonStyle
from sphinxcontrib.bibtex.style.referencing.super_ import SuperReferenceStyle
from sphinxcontrib.bibtex.plugin import (
    register_plugin as sphinxcontrib_register_plugin,
)


@dataclass
class SuperWithBrackets(SuperReferenceStyle):
    #: Bracket style for :cite:t: and variations.
    bracket_textual: BracketStyle = field(default_factory=BracketStyle)
    #: Bracket style for :cite:p: and variations.
    bracket_parenthetical: BracketStyle = field(default_factory=BracketStyle)
    #: Person style (applies to all relevant citation commands).
    person: PersonStyle = field(default_factory=PersonStyle)


sphinxcontrib_register_plugin(
    "sphinxcontrib.bibtex.style.referencing",
    "super_with_brackets",
    SuperWithBrackets,
)
