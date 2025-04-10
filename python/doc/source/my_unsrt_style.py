# ruff: noqa: D100, D101, D102
from pybtex.style.formatting.unsrt import Style as UnsrtStyle
from pybtex.style.template import sentence, href, join, optional, field
from pybtex.plugin import register_plugin


class MyUnsrtStyle(UnsrtStyle):
    def format_web_refs(self, e):
        # based on urlbst output.web.refs
        return sentence[
            optional[
                self.format_url(e),
                optional[" (visited on ", field("urldate"), ")"],
            ],
            optional[self.format_eprint(e)],
            optional[self.format_pubmed(e)],
            optional[self.format_doi(e)],
            optional[self.format_bibtex(e)],
        ]

    def format_bibtex(self, e):
        url = join[
            "https://iridia-ulb.github.io/references/index_bib.html#", e.key
        ]
        return href[url, "[BibTeX]"]


register_plugin("pybtex.style.formatting", "my_unsrt", MyUnsrtStyle)
