
EXTRA_DIST = debian/changelog debian/compat debian/control \
	debian/copyright debian/info debian/examples debian/rules \
	BUGS
SUBDIRS = src doc misc
ac_local = @ac_local@
version = @PACKAGE_VERSION@

install-data-hook:
	@if [ "x$(ac_local)" = "xyes" ]; then \
		echo "========================================"; \
		echo "miau is now installed to $(HOME)/miau/."; \
		if [ -f "$(HOME)/.miau/miaurc.new" ]; then \
			echo "New configuration file was installed as"; \
			echo "    $(HOME)/.miau/miaurc.new"; \
			echo "Please review it for changes."; \
		else \
			echo "Sample configuration file was installed as"; \
			echo "    $(HOME)/.miau/miaurc"; \
			echo "You must edit it before you can launch miau."; \
		fi; \
		echo "Once miaurc is all set up, you can launch miau by"; \
		echo "running $(HOME)/miau/miau-check"; \
		echo "========================================"; \
	fi
