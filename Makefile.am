ACLOCAL_AMFLAGS=-I m4
AUTOMAKE_OPTIONS = foreign
SUBDIRS = src

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA= icsc.pc

if HAVE_DOXYGEN
docs = $(top_srcdir)/docs

dist_man_MANS = $(docs)/doxyfile.stamp

$(docs)/doxyfile.stamp:
	cd $(docs) && $(DOXYGEN) Doxyfile
	echo Timestamp > $(docs)/doxyfile.stamp

CLEANFILES = $(docs)/doxyfile.stamp

all-local: $(docs)/doxyfile.stamp
clean-local:
	rm -rf $(top_srcdir)/docs/html
endif
