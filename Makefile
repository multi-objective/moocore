PACKAGEVERSION=0.1.4

.PHONY: clean check test pre-commit

pre-commit:
	pre-commit autoupdate
	pre-commit run -a

test, check:
	$(MAKE) -C r/ check
	$(MAKE) -C python/ test

clean:
	$(MAKE) -C c/ clean
	$(MAKE) -C r/ clean
	$(MAKE) -C python/ clean
	rm -rf *.Rcheck/

closeversion:
	git push origin :refs/tags/v$(PACKAGEVERSION) # Remove any existing tag
	git tag -f -a v$(PACKAGEVERSION) -m "Version $(PACKAGEVERSION)"
	git push --tags
