# Configuration file for the Sphinx documentation builder.
# ruff: noqa: D100, D103
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# See also: https://github.com/networkx/networkx/blob/main/doc/conf.py

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information
from datetime import date
import sphinx
import moocore

project = "moocore"
_full_version = moocore.__version__
release = _full_version.split("+", 1)[0]
version = ".".join(release.split(".")[:2])
year = date.today().year
author = "Manuel López-Ibáñez and Fergus Rooney"
copyright = f"2023-{year}, {author}"
html_site_root = f"https://multi-objective.github.io/{project}/python/"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

html_js_files = [
    "https://cdnjs.cloudflare.com/ajax/libs/require.js/2.3.4/require.min.js"
]

extensions = [
    "sphinx_design",  # grid directive
    "sphinx.ext.autosummary",  # Create neat summary tables for modules/classes/methods etc
    "sphinx.ext.autodoc",  # Core Sphinx library for auto html doc generation from docstrings
    "sphinx.ext.napoleon",
    "sphinx_autodoc_typehints",
    "sphinx.ext.intersphinx",  # Link to other project's documentation (see mapping below)
    "sphinx.ext.viewcode",  # Add a link to the Python source code for classes, functions etc.
    "sphinx_copybutton",  # A small sphinx extension to add a "copy" button to code blocks.
    "sphinx.ext.doctest",
    "sphinx.ext.extlinks",
    "sphinx.ext.mathjax",
    "myst_nb",
    "sphinx.ext.autosectionlabel",
    "sphinxcontrib.bibtex",
]

# -----------------------------------------------------------------------------
# Autosummary
# -----------------------------------------------------------------------------
autosummary_generate = True
# autosummary_generate_overwrite = False
# autosummary_ignore_module_all = False
# If true, the current module name will be prepended to all description
# unit titles (such as .. function::).
# add_module_names = True
# autosummary_imported_members = True  # Also documents imports in __init__.py
# Napoleon settings
napoleon_google_docstring = False
napoleon_numpy_docstring = True
napoleon_preprocess_types = True
napoleon_use_rtype = False
napoleon_include_init_with_doc = True
napoleon_use_param = True
napoleon_type_aliases = {
    "numpy.typing.ArrayLike": ":py:data:`~numpy.typing.ArrayLike`",
    "ArrayLike": ":py:data:`~numpy.typing.ArrayLike`",
}

bibtex_bibfiles = ["REFERENCES.bib"]
bibtex_reference_style = "super"
bibtex_default_style = "unsrt"

# If true, '()' will be appended to :func: etc. cross-reference text.
add_function_parentheses = True

autodoc_typehints = "none"  # Conflicts with sphinx_autodoc_typehints
typehints_document_rtype = True
typehints_use_rtype = False
typehints_defaults = "comma"
autodoc_type_aliases = {
    "ArrayLike": ":py:data:`~numpy.typing.ArrayLike`",
}

# nitpicky = True

# FIXME: This is not working!
logger = sphinx.util.logging.getLogger("sphinx.ext.autodoc")
members_to_watch = ["class", "exception", "function", "method", "attribute"]


def warn_undocumented_members(app, what, name, obj, options, lines):
    if what in members_to_watch and not lines:
        logger.warning(f"{what} is undocumented: {name}", type="autodoc")


def setup(app):
    app.connect("autodoc-process-docstring", warn_undocumented_members)


html_theme = "pydata_sphinx_theme"

# FIXME: Implement a version switch like NumPy.
# Set up the version switcher.  The versions.json is stored in the doc repo.
# if os.environ.get('CIRCLE_JOB', False) and \
#         os.environ.get('CIRCLE_BRANCH', '') != 'main':
#     # For PR, name is set to its ref
#     switcher_version = os.environ['CIRCLE_BRANCH']
# elif ".9999" in version:
#     switcher_version = "devdocs"
# else:
#     switcher_version = f"{version}"

html_theme_options = {
    "collapse_navigation": True,
    "header_links_before_dropdown": 6,
    #    "navigation_depth": 2,
    "show_prev_next": True,
    "use_edit_page_button": True,
    "github_url": f"https://github.com/multi-objective/{project}",
    # Add light/dark mode and documentation version switcher:
    # "navbar_end": ["theme-switcher", "version-switcher", "navbar-icon-links"],
    "navbar_end": ["theme-switcher", "navbar-icon-links"],
    # "icon_links": [
    #     {
    #         # Label for this link
    #         "name": "GitHub",
    #         # URL where the link will redirect
    #         "url": f"https://github.com/multi-objective/{project}",  # required
    #         # Icon class (if "type": "fontawesome"), or path to local image (if "type": "local")
    #         "icon": "fa-brands fa-square-github",
    #         # The type of image to be used (see below for details)
    #         "type": "fontawesome",
    #     }
    # ],
}
# Removes, from all docs, the copyright footer.
html_show_copyright = False

html_context = {
    "display_github": True,
    "github_user": "multi-objective",
    "github_repo": project,
    "github_version": "main",
    "doc_path": "doc",
}
# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]
html_css_files = [
    "css/custom.css",
]
templates_path = ["_templates"]
# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path .
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "**.ipynb_checkpoints",
    "_templates",
    "modules.rst",
    "source",
]
suppress_warnings = ["mystnb.unknown_mime_type"]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# html_permalinks_icon = "<span>#</span>"

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = "sphinx"

# Intersphinx mapping
intersphinx_mapping = {
    "python": ("https://docs.python.org/3/", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
    "neps": ("https://numpy.org/neps/", None),
    "matplotlib": ("https://matplotlib.org/stable/", None),
    "scipy": ("https://docs.scipy.org/doc/scipy/", None),
    "pandas": ("https://pandas.pydata.org/pandas-docs/stable/", None),
    "geopandas": ("https://geopandas.org/en/stable/", None),
    "pygraphviz": ("https://pygraphviz.github.io/documentation/stable/", None),
    "sphinx-gallery": ("https://sphinx-gallery.github.io/stable/", None),
    "nx-guides": ("https://networkx.org/nx-guides/", None),
    "sympy": ("https://docs.sympy.org/latest/", None),
}
