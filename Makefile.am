AM_CPPFLAGS = $(PGINCLUDES) $(all_includes)

lib_LTLIBRARIES = iplike.la
iplike_la_SOURCES = iplike.c
iplike_la_LDFLAGS = -module -avoid-version

if MAKE_RPMS
rpm: @RPM_TARGET@

.PHONY: rpm

$(RPM_TARGET): $(DISTFILES)
	${MAKE} dist
	-mkdir -p $(RPM_DIR)/SRPMS
	-mkdir -p `dirname $(RPM_TARGET)`
	$(RPM_PROG) $(RPM_ARGS) $(RPM_TARBALL)
	@echo "$(RPM_TARGET) created"
endif
