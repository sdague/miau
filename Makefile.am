
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

comp-test: 
	set -e
	@echo "Trying to compile (default) miau..."
	@./configure
	make clean
	make
	@echo "Trying to compile miau with everything but ipv6 enabled..."
	@./configure --enable-dccbounce --enable-automode --enable-releasenick \
			--enable-ctcp-replies --enable-mkpasswd \
			--enable-uptime --enable-logging  --enable-onconnect \
			--enable-enduserdebug --enable-pingstat --enable-debug \
			--enable-dumpstatus
	make clean
	make
	@echo "Cleaning up..."
	make clean
	@echo ""
	@echo -n "miau seems to comple on \""
	@echo -n `uname -s -r -m`
	@echo "\""

