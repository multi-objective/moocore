PACKAGEVERSION=0.1.5

.PHONY: default clean check test pre-commit

default: test

test, check:
	$(MAKE) -C r/ check
	$(MAKE) -C python/ test

clean:
	$(MAKE) -C c/ clean
	$(MAKE) -C r/ clean
	$(MAKE) -C python/ clean
	rm -rf *.Rcheck/

pre-commit:
	pre-commit autoupdate
	pre-commit run -a


closeversion:
	git push origin :refs/tags/v$(PACKAGEVERSION) # Remove any existing tag
	git tag -f -a v$(PACKAGEVERSION) -m "Version $(PACKAGEVERSION)"
	git push --tags
