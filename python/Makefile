# This file should be almost identical to
# https://github.com/multi-objective/mooplot/blob/main/python/Makefile
.PHONY : install build test doc clean docdeps pre-commit

install: build
	python3 -m pip install -e . --disable-pip-version-check

build:
	python3 -m build

test:
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

clean:
	$(MAKE) -C doc clean
	$(MAKE) -C src/moocore/libmoocore/ clean
	find . -name '__pycache__' | xargs $(RM) -r
	$(RM) -rf .pytest_cache .tox build src/*.egg-info/ doc/source/reference/generated
	$(RM) -f .coverage coverage.xml c_coverage.xml dist/*
