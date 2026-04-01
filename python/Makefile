# This file should be almost identical to
# https://github.com/multi-objective/mooplot/blob/main/python/Makefile

PYTHON := $(if $(VIRTUAL_ENV),$(VIRTUAL_ENV)/bin/python3,python3)
PIP := $(PYTHON) -m pip

.PHONY : install dev-install build test doc fastdoc clean docdeps pre-commit benchmarks

install: build
	$(PIP) install . --disable-pip-version-check

dev-install: build
	$(PIP) install -e . --disable-pip-version-check

build:
	$(PYTHON) -m build

test:
	$(MAKE) -C src/moocore/libmoocore/ clean
	tox

pre-commit:
	pre-commit autoupdate
	pre-commit run -a

docdeps:
	$(PIP) install .[docs] --disable-pip-version-check --quiet

show:
	$(MAKE) -C doc show

doc:
	$(MAKE) -C doc html

fastdoc:
	$(MAKE) -C doc html-noplot

clean:
	$(MAKE) -C doc clean
	$(MAKE) -C src/moocore/libmoocore/ clean
	find . -name '__pycache__' | xargs $(RM) -r
	$(RM) -r .pytest_cache .tox .ruff_cache build src/*.egg-info/ doc/source/auto_examples
	$(RM) .coverage coverage.xml c_coverage.xml dist/* src/moocore/*.so

benchmarks: install
	$(RM) ./benchmarks/*_bench-*.png
	cd benchmarks                             && \
	$(PYTHON) -m bench_epsilon                  && \
	$(PYTHON) -m bench_hv                       && \
	$(PYTHON) -m bench_hvapprox                 && \
	$(PYTHON) -m bench_igdplus                  && \
	$(PYTHON) -m bench_ndom                     && \
	$(PYTHON) -m bench_ndsort                   && \
	mv *.png -t ../doc/source/_static/bench/  && \
	cd ..
	$(MAKE) -C doc html
	$(MAKE) -C doc show
