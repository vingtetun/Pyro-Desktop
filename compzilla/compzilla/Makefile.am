
#
# Simple wrapper launch script
# 

bin_SCRIPTS=compzilla

#
# Extension contents
#

extensiondir = $(GECKO_LIBDIR)/extensions/compzilla@pyrodesktop.org
extension_DATA =			\
	$(srcdir)/chrome.manifest	\
	$(srcdir)/install.rdf

contentdir=$(extensiondir)/chrome/content
CONTENT_SRCDIR=$(srcdir)/chrome/content
content_DATA =					\
	$(CONTENT_SRCDIR)/pyrologo.png		\
	\
	$(CONTENT_SRCDIR)/atoms.js		\
	$(CONTENT_SRCDIR)/content.js		\
	$(CONTENT_SRCDIR)/debug.js		\
	$(CONTENT_SRCDIR)/frame.js		\
	$(CONTENT_SRCDIR)/jquery.1.1.2.js	\
	$(CONTENT_SRCDIR)/jquery.easing.1.1.js	\
	$(CONTENT_SRCDIR)/minmax.js		\
	$(CONTENT_SRCDIR)/overlay.js		\
	$(CONTENT_SRCDIR)/picker.js		\
	$(CONTENT_SRCDIR)/prompt.js		\
	$(CONTENT_SRCDIR)/scale.js		\
	$(CONTENT_SRCDIR)/start.js		\
	$(CONTENT_SRCDIR)/start.xul		\
	$(CONTENT_SRCDIR)/util.js		\
	$(CONTENT_SRCDIR)/windowStack.js	\
	$(CONTENT_SRCDIR)/workarea.js		\
	$(CONTENT_SRCDIR)/xprops.js

skindir=$(extensiondir)/chrome/skin
SKIN_SRCDIR=$(srcdir)/chrome/skin
skin_DATA =					\
	$(SKIN_SRCDIR)/compzilla.css		\
	$(SKIN_SRCDIR)/desktop-background.jpg	\
	$(SKIN_SRCDIR)/icon.png			\
	$(SKIN_SRCDIR)/minimize.png		\
	$(SKIN_SRCDIR)/maximize.png		\
	$(SKIN_SRCDIR)/close.png		\
	$(SKIN_SRCDIR)/meebo/*

endir=$(extensiondir)/chrome/locale/en-US
EN_SRCDIR=$(srcdir)/chrome/locale/en-US
en_DATA = $(EN_SRCDIR)/start.dtd

prefsdir = $(extensiondir)/defaults/preferences
PREFS_SRCDIR = $(srcdir)/defaults/preferences
prefs_DATA = $(PREFS_SRCDIR)/prefs.js


#
# The 'components' directory contains the generated .xpt typelibs,
# and libcompzilla.so.
#

componentdir = $(extensiondir)/components
component_DATA = $(IDL_XPT_FILES)
component_LTLIBRARIES = libcompzilla.la


#
# Use 'xpidl' to generate .h and .xpt files from .idl files.  
# NOTE: IDL files are not currently installed.
#

noinst_idldir = $(datadir)/idl/compzilla
IDL_SRCDIR=$(srcdir)/public
noinst_idl_DATA =					\
	$(IDL_SRCDIR)/compzillaIControl.idl		\
	$(IDL_SRCDIR)/compzillaIControlObserver.idl	\
	$(IDL_SRCDIR)/compzillaIWindow.idl		\
	$(IDL_SRCDIR)/compzillaIWindowObserver.idl   	\
	$(IDL_SRCDIR)/compzillaIRenderingContext.idl

IDL_H_DESTDIR = $(top_builddir)/compzilla/public
IDL_H_FILES = $(subst $(IDL_SRCDIR), $(IDL_H_DESTDIR), $(noinst_idl_DATA:.idl=.h))
IDL_XPT_FILES = $(IDL_H_FILES:.h=.xpt)

XPIDL_INCLUDES =				\
	-I$(GECKO_IDLDIR)			\
	-I$(IDL_SRCDIR)

%.h %.xpt: %.idl
	test -d "$(dir $@)" || mkdir "$(dir $@)"
	$(XPIDL) $(XPIDL_INCLUDES) -m header -e $*.h $<
	$(XPIDL) $(XPIDL_INCLUDES) -m typelib -e $*.xpt $<


EXTRA_DIST = 			\
	$(extension_DATA)	\
	$(content_DATA)		\
	$(skin_DATA)		\
	$(en_DATA)		\
	$(prefs_DATA)		\
	$(noinst_idl_DATA)

BUILT_SOURCES = 		\
	$(IDL_H_FILES) 		\
	$(IDL_XPT_FILES)

CLEANFILES = 			\
	$(IDL_H_FILES) 		\
	$(IDL_XPT_FILES)


#
# libcompzilla.so contains all C++ XPCOM components
#

if GECKO_USE_THEBES
GFX_CFLAGS=
GFX_LIBS=-L/home/toshok/src/mozilla/mozilla/dist/lib -lxpcomglue_s -lthebes
else
GFX_CFLAGS=-DMOZILLA_INTERNAL_API
GFX_LIBS=-lxpistub
endif

libcompzilla_la_LDFLAGS = -avoid-version -module -Wl,-Bsymbolic
libcompzilla_la_LIBADD = $(XEXTENSIONS_LIBS) $(NSPR_LIBS) -L$(GECKO_LIBDIR) $(GFX_LIBS)

### Link libthebes for dynamic FF builds
#libcompzilla_la_LIBADD = $(XEXTENSIONS_LIBS) $(NSPR_LIBS) -L$(GECKO_LIBDIR) -L/home/toshok/src/mozilla/mozilla/dist/lib -lxpcomglue_s libthebes.so

### Lots of debug output
# -DDEBUG -DDEBUG_SPEW -DDEBUG_EVENTS

libcompzilla_la_CPPFLAGS =			\
	-DDEBUG                                 \
	-DDEBUG_SPEW                            \
	-fshort-wchar				\
	-fno-rtti 				\
	-fno-exceptions 			\
	-shared					\
	\
	$(AM_CPPFLAGS)				\
	$(XEXTENSIONS_CFLAGS)			\
	$(GDK_CFLAGS)				\
	\
	-I$(GECKO_INCLUDEDIR)			\
	-include mozilla-config.h		\
	-I$(GECKO_INCLUDEDIR)/nspr		\
	-I$(GECKO_INCLUDEDIR)/caps		\
	-I$(GECKO_INCLUDEDIR)/content		\
	-I$(GECKO_INCLUDEDIR)/docshell		\
	-I$(GECKO_INCLUDEDIR)/dom		\
	-I$(GECKO_INCLUDEDIR)/gfx		\
	-I$(GECKO_INCLUDEDIR)/js		\
	-I$(GECKO_INCLUDEDIR)/layout		\
	-I$(GECKO_INCLUDEDIR)/locale		\
	-I$(GECKO_INCLUDEDIR)/necko		\
	-I$(GECKO_INCLUDEDIR)/pref		\
	-I$(GECKO_INCLUDEDIR)/string		\
	-I$(GECKO_INCLUDEDIR)/view		\
	-I$(GECKO_INCLUDEDIR)/widget		\
	-I$(GECKO_INCLUDEDIR)/xpcom		\
	-I$(GECKO_INCLUDEDIR)/xpconnect		\
	\
	-I$(IDL_H_DESTDIR)			\
	$(GFX_CFLAGS)

nodist_libcompzilla_la_SOURCES = $(IDL_H_FILES)

libcompzilla_la_SOURCES =				\
	$(srcdir)/src/compzillaControl.cpp		\
	$(srcdir)/src/compzillaControl.h		\
	$(srcdir)/src/compzillaModule.cpp		\
	$(srcdir)/src/compzillaRenderingContext.h 	\
	$(srcdir)/src/compzillaRenderingContext.cpp	\
	$(srcdir)/src/compzillaWindow.h			\
	$(srcdir)/src/compzillaWindow.cpp		\
	$(srcdir)/src/Debug.h				\
	$(srcdir)/src/nsKeycodes.h			\
	$(srcdir)/src/XAtoms.h

