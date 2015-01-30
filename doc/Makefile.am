
if HAVE_DOXYGEN

#directory = $(top_srcdir)/doc/man/man3/
directory = man/man3

dist_man_MANS = $(directory)/handlebars.h.3 \
                $(directory)/handlebars_context.h.3

$(directory)/handlebars.h.3: doxyfile.stamp
$(directory)/handlebars_context.3: doxyfile.stamp

doxyfile.stamp:
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp
clean-local:
	rm -rf $(top_srcdir)/doc/html
	rm -rf $(top_srcdir)/doc/man

endif
