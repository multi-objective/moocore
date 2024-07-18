PACKAGEVERSION=$(shell sh -c 'grep -F "Version: " DESCRIPTION | cut -f2 -d" "')
PACKAGE=$(shell sh -c 'grep -F "Package: " DESCRIPTION | cut -f2 -d" "')
REALVERSION=$(PACKAGEVERSION)$(REVNUM)

BINDIR=$(CURDIR)/..
INSTALL_FLAGS="--with-keep.source"
PACKAGEDIR=$(CURDIR)
Reval=R --slave -e

define Rsed
	R --slave --vanilla -e 'f <- "$(1)"; txt <- sub($(2), $(3), perl=TRUE, readLines(f)); writeLines(txt, f)'
endef

## Do we have git?
ifeq ($(shell sh -c 'which git 1> /dev/null 2>&1 && echo y'),y)
  ## Is this a working copy?
  ifeq ($(shell sh -c 'LC_ALL=C  git describe --first-parent --always | grep -q "[0-9a-z]\+$$" 2>&1 && echo y'),y)
    $(shell sh -c 'LC_ALL=C  git describe --dirty --first-parent --always --exclude "*" > git_version')
  endif
endif
## Set version information:
REVNUM = $(shell sh -c 'cat git_version 2> /dev/null')

.PHONY: build check clean install pdf submit cran winbuild help gendoc pkgdown

help :
	@echo 'This makefile has the following targets                  '
	@echo '   build     build $(PACKAGE) at $(BINDIR)		'
	@echo '   cran      check with --as-cran			'
	@echo '   check     check with --timings			'
	@echo '   check TEST=x  run test called test-x.R                '
	@echo '   pdf       build $(PACKAGE).pdf			'
	@echo '   winbuild  send to http\://win-builder.r-project.org/	'
	@echo '   submit    submit to CRAN (see DEVEL-README first!)    '
	@echo '   clean     cleanup    '

install: build
	cd $(BINDIR) && R CMD INSTALL $(INSTALL_FLAGS) $(PACKAGE)_$(PACKAGEVERSION).tar.gz

setup:
	Rscript -e 'if(!require(devtools)) install.packages(c("devtools", "xfun")); devtools::install_deps(upgrade="never")'

gendoc: $(PACKAGEDIR)/man/$(PACKAGE)-package.Rd

NAMESPACE $(PACKAGEDIR)/man/$(PACKAGE)-package.Rd: $(PACKAGEDIR)/R/*.R
	$(Reval) 'pkgbuild::compile_dll();devtools::document()'

pkgdown: gendoc
	$(Reval) 'pkgdown::build_site(run_dont_run = TRUE)'

build:
	@$(MAKE) gendoc
	cd $(BINDIR) && R CMD build $(PACKAGEDIR)

closeversion:
	git push origin :refs/tags/v$(PACKAGEVERSION) # Remove any existing tag
	git tag -f -a v$(PACKAGEVERSION) -m "Version $(PACKAGEVERSION)"
	git push --tags

releasebuild: clean
	@$(MAKE) gendoc
	cp .Rbuildignore .Rbuildignore.backup
	cat .Rbuildignore.cran >> .Rbuildignore
	cd $(BINDIR) &&	R CMD build $(PACKAGEDIR) && tar -atvf $(PACKAGE)_$(PACKAGEVERSION).tar.gz
	mv .Rbuildignore.backup .Rbuildignore
	Rscript --vanilla do.R info "$(BINDIR)/$(PACKAGE)_$(PACKAGEVERSION).tar.gz"

releasecheck:
	$(Reval) 'urlchecker::url_check()'
	$(MAKE) releasebuild
	$(MAKE) cran_
	$(MAKE) winbuild_ # $(MAKE) macbuild # rhub is broken

cran : build
cran cran_:
	cd $(BINDIR) && _R_CHECK_FORCE_SUGGESTS_=false _R_OPTIONS_STRINGS_AS_FACTORS_=false R CMD check --as-cran $(PACKAGE)_$(PACKAGEVERSION).tar.gz

check: build
ifdef TEST
	_R_CHECK_FORCE_SUGGESTS_=false NOT_CRAN=true $(Reval) 'devtools::test(filter="$(TEST)")'
else
	cd $(BINDIR) && (_R_CHECK_FORCE_SUGGESTS_=false NOT_CRAN=true R CMD check --run-donttest --run-dontrun --timings $(PACKAGE)_$(PACKAGEVERSION).tar.gz; cat $(PACKAGE).Rcheck/$(PACKAGE)-Ex.timings)
endif

clean:
	cd $(PACKAGEDIR) && ./cleanup

pdf:
	$(RM) $(BINDIR)/$(PACKAGE).pdf
	cd $(BINDIR) && R CMD Rd2pdf --no-preview --batch --output=$(PACKAGE).pdf $(PACKAGEDIR)

submit: releasebuild
	Rscript --vanilla do.R submit "$(BINDIR)/$(PACKAGE)_$(PACKAGEVERSION).tar.gz"


winbuild: releasebuild
winbuild winbuild_:
	$(Reval) "devtools::check_win_release()"
	$(Reval) "devtools::check_win_devel()"

covr:
	$(Reval) "Sys.setenv(NOT_CRAN='true');covr::report(covr::package_coverage(type='all', quiet=FALSE), browse=TRUE)"
