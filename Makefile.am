
EXTRA_DIST = debian/changelog debian/compat debian/control \
	debian/copyright debian/files debian/info debian/miau.examples \
	debian/miau.postinst.debhelper debian/miau.prerm.debhelper \
	debian/miau.substvars debian/rules
SUBDIRS = src doc misc
ac_local = @ac_local@

install-data-hook:
	@if test $(ac_local) = yes; then \
	    echo "============================================="; \
	    echo "Edit $(HOME)/.miau/miaurc!"; \
	    echo "Additionally a $(HOME)/miau/-dir has been"; \
	    echo "created and the miau-binary, documentation and"; \
	    echo "some other stuff has been put there."; \
	    echo "============================================="; \
	fi
