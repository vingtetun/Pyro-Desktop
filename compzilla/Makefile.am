
SUBDIRS = compzilla

SRC_DIR=`pwd`
PROFILE_DIR=`readlink -f ${HOME}/.mozilla/firefox/*.pyrodesktop`

debug:
	./run-xephyr.sh &
	DISPLAY=:9 NSPR_LOG_MODULES="compzilla:10" \
	$(GECKO_EXEC_PREFIX)/run-mozilla.sh -g ./firefox-bin -chrome chrome://compzilla/content/start.xul -P pyrodesktop

# Check the library for undefined symbol that prevent it to load
check-dev:
	$(GECKO_EXEC_PREFIX)/run-mozilla.sh `which ldd` -r compzilla/.libs/libcompzilla.so

#XXX do symbolic link instead of copying the .so/.xpt
install-dev:
	echo ${SRC_DIR}/${SUBDIRS} > ${PROFILE_DIR}/extensions/compzilla@pyrodesktop.org
	mkdir -p ${SUBDIRS}/components
	cp ${SUBDIRS}/.libs/libcompzilla.so ${SUBDIRS}/components/
	cp ${SUBDIRS}/public/*.xpt ${SUBDIRS}/components/

run:
	./run-xephyr.sh &
	DISPLAY=:9 NSPR_LOG_MODULES="compzilla:10" \
	$(FIREFOX) -chrome chrome://compzilla/content/start.xul -P pyrodesktop -no-remote



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

