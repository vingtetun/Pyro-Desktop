

extensiondir = $(GECKO_LIBDIR)/extensions/compzilla@beatniksoftware.com
EXTENSION_SRCDIR=$(srcdir)
extension_DATA =				\
	$(EXTENSION_SRCDIR)/chrome.manifest	\
	$(EXTENSION_SRCDIR)/install.rdf

contentdir=$(extensiondir)/chrome/content
CONTENT_SRCDIR=$(EXTENSION_SRCDIR)/chrome/content
content_DATA =					\
	$(CONTENT_SRCDIR)/overlay.js		\
	$(CONTENT_SRCDIR)/overlay.xul		\
	$(CONTENT_SRCDIR)/start.js		\
	$(CONTENT_SRCDIR)/start.xul

skindir=$(extensiondir)/chrome/skin
SKIN_SRCDIR=$(EXTENSION_SRCDIR)/chrome/skin
skin_DATA =					\
	$(SKIN_SRCDIR)/compzilla.css		\
	$(SKIN_SRCDIR)/desktop.html		\
	$(SKIN_SRCDIR)/desktop-background.jpg

endir=$(extensiondir)/chrome/locale/en-US
EN_SRCDIR=$(EXTENSION_SRCDIR)/chrome/locale/en-US
en_DATA = $(EN_SRCDIR)/overlay.dtd

prefsdir = $(extensiondir)/defaults/preferences
PREFS_SRCDIR = $(EXTENSION_SRCDIR)/defaults/preferences
prefs_DATA = $(PREFS_SRCDIR)/prefs.js


#
# The 'components' directory contains the generated .xpt typelibs,
# libcompzilla.so, and compzillaWindowManager.js
#

componentdir = $(extensiondir)/components
COMPONENT_SRCDIR = $(EXTENSION_SRCDIR)/components
component_DATA = $(IDL_XPT_FILES)				\
		 $(COMPONENT_SRCDIR)/compzillaWindowManager.js
component_LTLIBRARIES = libcompzilla.la


#
# Use 'xpidl' to generate .h and .xpt files from .idl files
#

XPIDL_INCLUDES =				\
	-I$(GECKO_IDLDIR)			\
	-I$(srcdir)/public

%.h: %.idl
	$(XPIDL) $(XPIDL_INCLUDES) -m header -e $@ $<

%.xpt: %.idl
	$(XPIDL) $(XPIDL_INCLUDES)  -m typelib -e $@ $<

IDL_FILES =						\
	$(srcdir)/public/compzillaIExtension.idl	\
	$(srcdir)/public/compzillaIControl.idl		\
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

libcompzilla_la_LDFLAGS = -avoid-version -module
libcompzilla_la_LIBADD = $(XEXTENSIONS_LIBS)

libcompzilla_la_CPPFLAGS =			\
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
	-I$(GECKO_INCLUDEDIR)/widget		\
	-I$(GECKO_INCLUDEDIR)/xpcom		\
	-I$(GECKO_INCLUDEDIR)/xpconnect		\
	\
	-I$(top_srcdir)/gecko-sdk/include	\
	-I$(srcdir)/public

nodist_libcompzilla_la_SOURCES = $(IDL_H_FILES)

libcompzilla_la_SOURCES =				\
	$(srcdir)/src/compzillaExtension.cpp		\
	$(srcdir)/src/compzillaExtension.h		\
	$(srcdir)/src/compzillaControl.cpp		\
	$(srcdir)/src/compzillaControl.h		\
	$(srcdir)/src/compzillaModule.cpp		\
	$(srcdir)/src/compzillaRenderingContext.h 	\
	$(srcdir)/src/compzillaRenderingContext.cpp

