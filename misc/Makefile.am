
bin_SCRIPTS = miau-check miau-rotatelog
data_DATA = miaurc
EXTRA_DIST = miaurc.in miau-check miau-rotatelog

install-dataDATA: $(data_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) "$(DESTDIR)$(datadir)"
	@if test "x$(ac_local)" = "xyes"; then \
	    chmod 700 "$(DESTDIR)$(datadir)" ; \
	fi
	@list="$(data_DATA)"; for p in $$list; do \
	    f="$(DESTDIR)$(datadir)/$$p" ; \
	    if test "x$(ac_local)" = "xyes"; then \
	        if test -f "$$f"; then \
		    f="$$f.new" ; \
		fi; \
	    fi; \
	    if test -f "$(srcdir)/$$p" ; then \
		$(INSTALL_DATA) "$(srcdir)/$$p" "$$f" ; \
	    else if test -f "$$p"; then \
		$(INSTALL_DATA) "$$p" "$$f" ; \
	    fi; fi; \
	done
