# This file should be almost identical to
# https://github.com/multi-objective/mooplot/blob/main/python/Makefile
.PHONY : install dev-install build test doc fastdoc clean docdeps pre-commit benchmarks

install: build
	python3 -m pip install . --disable-pip-version-check

dev-install: build
	python3 -m pip install -e . --disable-pip-version-check

build:
	python3 -m build

test:
	$(MAKE) -C src/moocore/libmoocore/ clean
	tox

pre-commit:
	pre-commit autoupdate
	pre-commit run -a

docdeps:
	python3 -m pip install .[docs] --disable-pip-version-check --quiet

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
	python3 bench_epsilon.py                  && \
	python3 bench_hv.py                       && \
	python3 bench_hvapprox.py                 && \
	python3 bench_igdplus.py                  && \
	python3 bench_ndom.py                     && \
	python3 bench_ndsort.py                   && \
	mv *.png -t ../doc/source/_static/bench/  && \
	cd ..
	$(MAKE) -C doc html
	$(MAKE) -C doc show
