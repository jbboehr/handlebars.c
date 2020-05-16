
if DOXYGEN

dist_man_MANS = man html
#dist_html_MANS = html

man: doxyfile.stamp
html: doxyfile.stamp

doxyfile.stamp:
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp
clean-local:
	rm -rf $(top_srcdir)/doc/html
	rm -rf $(top_srcdir)/doc/man

endif
