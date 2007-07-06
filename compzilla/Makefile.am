
SUBDIRS = compzilla

debug:
	$(FIREFOX) -g -chrome chrome://compzilla/content/start.xul -P dev

run:
	$(FIREFOX) -chrome chrome://compzilla/content/start.xul -P dev


#
# Build the installable XPI package
#

xpi: all $(distdir).xpi

$(distdir).xpi:
	-mkdir /tmp/xpi-stage
	$(MAKE) DESTDIR=/tmp/xpi-stage install

	cd /tmp/xpi-stage/$(GECKO_LIBDIR)/extensions/compzilla@pyrodesktop.org && \
	    zip -q9 -r /tmp/xpi-stage/$@ *

	mv /tmp/xpi-stage/$@ $@

	-rm -r /tmp/xpi-stage

dist-hook: xpi
CLEANFILES = $(distdir).xpi
