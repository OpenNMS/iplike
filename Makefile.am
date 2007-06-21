ACLOCAL_AMFLAGS = -I .
AM_CPPFLAGS = $(PGINCLUDES) $(all_includes)
distdir = $(PACKAGE)-$(VERSION)-$(RELEASE)

lib_LTLIBRARIES = iplike.la
iplike_la_SOURCES = iplike.c
iplike_la_LDFLAGS = -module -avoid-version

sbin_SCRIPTS = install_iplike.sh

MAINTAINERCLEANFILES = aclocal.m4 config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing *~

if MAKE_RPMS
rpm: @RPM_TARGET@

.PHONY: rpm

EXTRA_DIST = iplike.spec

$(RPM_TARGET): $(DISTFILES)
	${MAKE} dist
	-mkdir -p $(RPM_DIR)/SRPMS
	-mkdir -p `dirname $(RPM_TARGET)`
	$(RPMBUILD_PROG) $(RPM_ARGS) $(RPM_TARBALL)
	@echo "$(RPM_TARGET) created"
endif
