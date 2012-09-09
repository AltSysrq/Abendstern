doc_DATA=README COPYING
if ENABLE_FONTS
SUBDIRS=src fonts
else
SUBDIRS=src
endif
install-data-hook: remove-backups
	mkdir -p $(pkgdatadir)
	cp -R abendstern.default.rc hangar.default data fonts images tcl \
	  shaders version $(pkgdatadir)

uninstall-local:
	rm -Rf $(pkgdatadir)

remove-backups:
	find . -name '*~' | xargs rm -f

dist_data_DATA:
