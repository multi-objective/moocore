# This file should be almost identical to
# https://github.com/multi-objective/mooplot/blob/main/python/Makefile
.PHONY : install dev-install build test doc fastdoc clean docdeps pre-commit

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
	python3 -m pip install -r requirements_dev.txt --disable-pip-version-check --quiet

show:
	$(MAKE) -C doc show

doc:
	$(MAKE) -C doc clean html

fastdoc:
	$(MAKE) -C doc clean html-noplot

clean:
	$(MAKE) -C doc clean
	$(MAKE) -C src/moocore/libmoocore/ clean
	find . -name '__pycache__' | xargs $(RM) -r
	$(RM) -r .pytest_cache .tox .ruff_cache build src/*.egg-info/ doc/source/auto_examples
	$(RM) .coverage coverage.xml c_coverage.xml dist/* src/moocore/*.so
