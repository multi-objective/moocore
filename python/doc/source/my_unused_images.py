# ruff: noqa: D100
from pathlib import Path
from docutils import nodes
from sphinx.util import logging

logger = logging.getLogger(__name__)


def check_unused_images(app, exception):
    """Warn about unused images (ignore Sphinx-gallery examples)."""
    if exception:
        return

    used = set()

    for docname in app.env.found_docs:
        doctree = app.env.get_doctree(docname)
        document = Path(app.env.doc2path(docname)).resolve()

        for image in doctree.findall(nodes.image):
            uri = image["uri"]

            if "://" in uri or uri.startswith("data:"):
                continue

            # Sphinx stores the URI relative to the source directory, which is
            # not the folder of the document for documents in subfolders.
            used.add((document.parent / uri).resolve())
            used.add((Path(app.srcdir) / uri).resolve())

    for image in Path(app.srcdir).rglob("*.png"):
        if "auto_examples" in image.parts:
            continue

        if image.resolve() not in used:
            logger.warning("Unused image: %s", image.relative_to(app.srcdir))


# ruff: noqa: D103
def setup(app):
    app.connect("build-finished", check_unused_images)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
    }
