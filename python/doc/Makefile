# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= LANG=C sphinx-build
SOURCEDIR     = source
BUILDDIR      = _build

# For merging a documentation archive into a git checkout of numpy/doc
# Turn a tag like v1.18.0 into 1.18
# Use sed -n -e 's/patttern/match/p' to return a blank value if no match
# TAG ?= $(shell git describe --tag | sed -n -e's,v\([1-9]\.[0-9]*\)\.[0-9].*,\1,p')

root_dir:=$(abspath $(shell dirname $(firstword $(MAKEFILE_LIST))))

.PHONY: help Makefile

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

show:
	@python3 -c "import webbrowser; webbrowser.open_new_tab('file://$(root_dir)/$(BUILDDIR)/html/index.html')"

html-noplot:
	@$(SPHINXBUILD) -D plot_gallery=0 -b html $(ALLSPHINXOPTS) "$(SOURCEDIR)" "$(BUILDDIR)/html"
	@echo "Build finished. The HTML pages are in $(BUILDDIR)/html."

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" -WT --keep-going -d $(BUILDDIR)/doctrees $(SPHINXOPTS) $(O)
