
bin_SCRIPTS=compzilla

extensiondir = $(GECKO_LIBDIR)/extensions/compzilla@pyrodesktop.org
EXTENSION_SRCDIR=$(srcdir)
extension_DATA =				\
	$(EXTENSION_SRCDIR)/chrome.manifest	\
	$(EXTENSION_SRCDIR)/install.rdf

contentdir=$(extensiondir)/chrome/content
CONTENT_SRCDIR=$(EXTENSION_SRCDIR)/chrome/content
content_DATA =					\
	$(CONTENT_SRCDIR)/start.js		\
	$(CONTENT_SRCDIR)/start.xul		\
	$(CONTENT_SRCDIR)/atoms.js		\
	$(CONTENT_SRCDIR)/content.js		\
	$(CONTENT_SRCDIR)/debug.js		\
	$(CONTENT_SRCDIR)/frame.js		\
	$(CONTENT_SRCDIR)/overlay.js		\
	$(CONTENT_SRCDIR)/windowStack.js	\
	$(CONTENT_SRCDIR)/scale.js

skindir=$(extensiondir)/chrome/skin
SKIN_SRCDIR=$(EXTENSION_SRCDIR)/chrome/skin
skin_DATA =					\
	$(SKIN_SRCDIR)/compzilla.css		\
	$(SKIN_SRCDIR)/desktop.html		\
	$(SKIN_SRCDIR)/desktop-background.jpg	\
	$(SKIN_SRCDIR)/icon.png			\
	$(SKIN_SRCDIR)/minimize.png		\
	$(SKIN_SRCDIR)/maximize.png		\
	$(SKIN_SRCDIR)/close.png

endir=$(extensiondir)/chrome/locale/en-US
EN_SRCDIR=$(EXTENSION_SRCDIR)/chrome/locale/en-US
en_DATA = $(EN_SRCDIR)/start.dtd

prefsdir = $(extensiondir)/defaults/preferences
PREFS_SRCDIR = $(EXTENSION_SRCDIR)/defaults/preferences
prefs_DATA = $(PREFS_SRCDIR)/prefs.js


#
# The 'components' directory contains the generated .xpt typelibs,
# libcompzilla.so
#

componentdir = $(extensiondir)/components
component_DATA = $(IDL_XPT_FILES)
component_LTLIBRARIES = libcompzilla.la


#
# Use 'xpidl' to generate .h and .xpt files from .idl files
#

XPIDL_INCLUDES =				\
	-I$(GECKO_IDLDIR)			\
	-I$(srcdir)/public

SUFFIXES= .idl .xpt

.idl.h:
	$(XPIDL) $(XPIDL_INCLUDES) -m header -e $@ $<

.idl.xpt:
	$(XPIDL) $(XPIDL_INCLUDES)  -m typelib -e $@ $<

IDL_FILES =						\
	$(srcdir)/public/compzillaIControl.idl		\
	$(srcdir)/public/compzillaIWindow.idl		\
	$(srcdir)/public/compzillaIWindowEvents.idl	\
	$(srcdir)/public/compzillaIWindowManager.idl    \
	$(srcdir)/public/compzillaIRenderingContext.idl

IDL_H_FILES = $(IDL_FILES:.idl=.h)
IDL_XPT_FILES = $(IDL_FILES:.idl=.xpt)

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
GFX_LIBS=-lthebes
else
GFX_LIBS=-lgkgfx
endif

libcompzilla_la_LDFLAGS = -avoid-version -module
libcompzilla_la_LIBADD = $(XEXTENSIONS_LIBS) -L$(GECKO_LIBDIR) $(GFX_LIBS) -lxpcom_core

libcompzilla_la_CPPFLAGS =			\
	-DDEBUG					\
	-DWITH_SPEW				\
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
	-I$(srcdir)/public

nodist_libcompzilla_la_SOURCES = $(IDL_H_FILES)

libcompzilla_la_SOURCES =				\
	$(srcdir)/src/compzillaEventManager.cpp		\
	$(srcdir)/src/compzillaEventManager.h		\
	$(srcdir)/src/compzillaControl.cpp		\
	$(srcdir)/src/compzillaControl.h		\
	$(srcdir)/src/compzillaModule.cpp		\
	$(srcdir)/src/compzillaRenderingContext.h 	\
	$(srcdir)/src/compzillaRenderingContext.cpp	\
	$(srcdir)/src/compzillaWindow.h			\
	$(srcdir)/src/compzillaWindow.cpp		\
	$(srcdir)/src/compzillaWindowEvents.h		\
	$(srcdir)/src/compzillaWindowEvents.cpp		\
	$(srcdir)/src/nsKeycodes.h			\
	$(srcdir)/src/XAtoms.h

