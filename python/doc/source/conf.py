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

# Set plotly renderer to capture _repr_html_ for sphinx-gallery
try:
    import plotly.io
except ImportError:
    pass
else:
    plotly.io.renderers.default = "sphinx_gallery"

import os
import sys

# Prevent ruff from deleting seemingly unused imports
sys.path.insert(0, os.path.abspath("../../src/moocore/"))
import moocore

sys.path.insert(0, os.path.abspath("."))
import my_unsrt_style  # noqa: F401

project = "moocore"
_full_version = moocore.__version__
release = _full_version  # _full_version.split("+", 1)[0]
version = _full_version  # ".".join(release.split(".")[:2])
year = date.today().year
# Can we get this from pyproject.toml ?
author = "Manuel López-Ibáñez and Fergus Rooney"
copyright = f"2024-{year}, {author}"
html_site_root = f"https://multi-objective.github.io/{project}/python/"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

html_js_files = [
    "https://cdnjs.cloudflare.com/ajax/libs/require.js/2.3.4/require.min.js"
]

extensions = [
    "sphinx_design",  # grid directive
    "sphinx.ext.autodoc",  # Core Sphinx library for auto html doc generation from docstrings
    "sphinx.ext.viewcode",  # Add a link to the Python source code for classes, functions etc.
    "sphinx.ext.doctest",
    "sphinx.ext.intersphinx",  # Link to other project's documentation (see mapping below)
    "sphinx.ext.extlinks",
    "sphinx.ext.napoleon",
    "sphinx_autodoc_typehints",
    "sphinx.ext.autosummary",  # Create neat summary tables for modules/classes/methods etc
    "sphinx_copybutton",  # A small sphinx extension to add a "copy" button to code blocks.
    "sphinx.ext.mathjax",
    # "sphinx.ext.autosectionlabel", DO NOT USE: causes duplicated labels.
    "sphinxcontrib.bibtex",
    "sphinx_gallery.gen_gallery",
    "matplotlib.sphinxext.plot_directive",
]

# https://sphinx-copybutton.readthedocs.io/en/latest/use.html
copybutton_exclude = ".linenos, .gp, .go"
copybutton_only_copy_prompt_lines = True
copybutton_remove_prompts = True
copybutton_copy_empty_lines = False

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
napoleon_include_init_with_doc = False
napoleon_use_param = True
napoleon_type_aliases = {
    "numpy.typing.ArrayLike": ":py:data:`~numpy.typing.ArrayLike`",
    "ArrayLike": ":py:data:`~numpy.typing.ArrayLike`",
}

bibtex_bibfiles = ["REFERENCES.bib"]
bibtex_reference_style = "super"
# bibtex_default_style = "unsrt"
bibtex_default_style = "my_unsrt"
bibtex_cite_id = "cite-{bibliography_count}-{key}"
bibtex_footcite_id = "footcite-{key}"
bibtex_bibliography_id = "bibliography-{bibliography_count}"
bibtex_footbibliography_id = "footbibliography-{footbibliography_count}"

# If true, '()' will be appended to :func: etc. cross-reference text.
add_function_parentheses = True

autodoc_typehints = "none"  # Conflicts with sphinx_autodoc_typehints
typehints_document_rtype = True
typehints_use_rtype = False
typehints_defaults = "comma"
always_use_bars_union = True
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
    "sidebar_includehidden": True,
    "github_url": f"https://github.com/multi-objective/{project}",
    "collapse_navigation": True,
    "navigation_with_keys": False,
    "navbar_align": "left",
    "header_links_before_dropdown": 5,
    "show_toc_level": 2,  # On this page sidebar
    "show_prev_next": True,
    "use_edit_page_button": True,
    # Add light/dark mode and documentation version switcher:
    # "navbar_end": ["theme-switcher", "version-switcher", "navbar-icon-links"],
    "navbar_end": ["theme-switcher", "navbar-icon-links"],
    "icon_links": [
        {
            "name": "PyPI",
            "url": f"https://pypi.org/project/{project}",
            "icon": "fa-solid fa-box",
        },
    ],
    # https://sphinx-gallery.github.io/stable/advanced.html#using-sphinx-gallery-sidebar-components
    "secondary_sidebar_items": [
        "page-toc",
        "sg_download_links",
        "sg_launcher_links",
    ],
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
html_show_sourcelink = False
# Removes, from all docs, the copyright footer.
html_show_copyright = False

html_context = {
    "display_github": True,
    "github_user": "multi-objective",
    "github_repo": project,
    "github_version": "main",
    "doc_path": "python/doc/source",
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
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "**.ipynb_checkpoints",
    "_templates",
    "modules.rst",
    "source",
    # Exclude .py and .ipynb files in auto_examples generated by sphinx-gallery.
    # This is to prevent sphinx from complaining about duplicate source files.
    "auto_examples/*.ipynb",
    "auto_examples/*.py",
]
suppress_warnings = ["mystnb.unknown_mime_type"]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# html_permalinks_icon = "<span>#</span>"

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = "sphinx"

# Intersphinx mapping
intersphinx_mapping = {
    "matplotlib": ("https://matplotlib.org/stable/", None),
    "mooplot": ("https://multi-objective.github.io/mooplot/python/", None),
    "neps": ("https://numpy.org/neps/", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
    "nx-guides": ("https://networkx.org/nx-guides/", None),
    "pandas": ("https://pandas.pydata.org/pandas-docs/stable/", None),
    "python": ("https://docs.python.org/3/", None),
    "scipy": ("https://docs.scipy.org/doc/scipy/", None),
}

# From https://github.com/scikit-learn/scikit-learn/blob/main/doc/conf.py
sphinx_gallery_conf = {
    "examples_dirs": "../../examples",
    "gallery_dirs": "auto_examples",
    # Directory where function/class granular galleries are stored.
    "backreferences_dir": "reference/generated/backreferences",
    # Modules for which function/class level galleries are created.
    "doc_module": (project),
    # Regexes to match objects to exclude from implicit backreferences.
    # The default option is an empty set, i.e. exclude nothing.
    # To exclude everything, use: '.*'
    "exclude_implicit_doc": {r"pyplot\.show"},
    "show_memory": False,
    "reference_url": {project: None},
    # "subsection_order": SubSectionTitleOrder("../examples"),
    # "within_subsection_order": SKExampleTitleSortKey,
    # "binder": {
    #     "org": "scikit-learn",
    #     "repo": "scikit-learn",
    #     "binderhub_url": "https://mybinder.org",
    #     "branch": binder_branch,
    #     "dependencies": "./binder/requirements.txt",
    #     "use_jupyter_lab": True,
    # },
    # avoid generating too many cross links
    "inspect_global_variables": False,
    "remove_config_comments": True,
    "matplotlib_animations": True,
    # "plot_gallery": "True",
    # "recommender": {"enable": True, "n_examples": 5, "min_df": 12},
    # "reset_modules": ("matplotlib", "seaborn"),
}
# if with_jupyterlite:
#     sphinx_gallery_conf["jupyterlite"] = {
#         "notebook_modification_function": notebook_modification_function
#     }
