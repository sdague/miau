
bin_SCRIPTS = miau-check
data_DATA = miaurc
EXTRA_DIST = miaurc.in miau-check
ac_local = @ac_local@

install-dataDATA: $(data_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) "$(DESTDIR)$(datadir)"
	@if [ "x$(ac_local)" = "xyes" ]; then \
		chmod 700 "$(DESTDIR)$(datadir)"; \
	fi
	@list="$(data_DATA)"; \
	for p in $$list; do \
		f="$(DESTDIR)$(datadir)/$$p"; \
		if [ "x$(ac_local)" = "xyes" ]; then \
			if [ -f "$$f" ]; then \
				f="$$f.new"; \
			fi; \
		fi; \
		if [ -f "$(srcdir)/$$p" ]; then \
			$(INSTALL_DATA) "$(srcdir)/$$p" "$$f"; \
		else \
			if [ -f "$$p" ]; then \
				$(INSTALL_DATA) "$$p" "$$f"; \
			fi; \
		fi; \
	done
