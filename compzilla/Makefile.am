
SUBDIRS = compzilla

debug:
	./run-xephyr.sh &
	DISPLAY=:9 NSPR_LOG_MODULES="compzilla:10" \
	$(FIREFOX) -g -chrome chrome://compzilla/content/start.xul -P dev

run:
	./run-xephyr.sh &
	DISPLAY=:9 NSPR_LOG_MODULES="compzilla:10" \
	$(FIREFOX) -chrome chrome://compzilla/content/start.xul -P dev -no-remote


#
# Build the installable XPI package
#

XPINAME="pyro-$(VERSION)-$(FIREFOX_SHORTNAME).xpi"

xpi: all $(XPINAME)

$(XPINAME):
	-mkdir /tmp/xpi-stage
	$(MAKE) DESTDIR=/tmp/xpi-stage install

	cd /tmp/xpi-stage/$(GECKO_LIBDIR)/extensions/compzilla@pyrodesktop.org && \
	    zip -q9 -r /tmp/xpi-stage/$@ *

	mv /tmp/xpi-stage/$@ $@

	-rm -r /tmp/xpi-stage

dist-hook: xpi
CLEANFILES = $(XPINAME)
