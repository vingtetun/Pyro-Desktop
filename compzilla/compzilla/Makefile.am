
GECKO_PRIVATE_INCLUDEDIR=$(top_srcdir)/gecko-sdk/include


extensiondir=$(pkglibdir)/firefox
EXTENSION_SRCDIR=$(srcdir)
extension_DATA =				\
	$(EXTENSION_SRCDIR)/chrome.manifest	\
	$(EXTENSION_SRCDIR)/install.rdf

contentdir=$(extensiondir)/chrome/content
CONTENT_SRCDIR=$(EXTENSION_SRCDIR)/chrome/content
content_DATA =					\
	$(CONTENT_SRCDIR)/overlay.js		\
	$(CONTENT_SRCDIR)/overlay.xul

endir=$(extensiondir)/chrome/locale/en-US
EN_SRCDIR=$(EXTENSION_SRCDIR)/chrome/locale/en-US
en_DATA = $(EN_SRCDIR)/overlay.dtd

prefsdir=$(extensiondir)/defaults/preferences
PREFS_SRCDIR=$(EXTENSION_SRCDIR)/defaults/preferences
prefs_DATA = $(PREFS_SRCDIR)/prefs.js


IDL_FILES =						\
	$(srcdir)/public/compzillaIExtension.idl	\
	$(srcdir)/public/compzillaIControl.idl
IDL_H_FILES = $(IDL_FILES:.idl=.h)
IDL_XPT_FILES = $(IDL_FILES:.idl=.xpt)

BUILT_SOURCES = $(IDL_H_FILES) $(IDL_XPT_FILES) 
CLEANFILES = $(IDL_H_FILES) $(IDL_XPT_FILES)


componentdir=$(extensiondir)/components
component_LTLIBRARIES = libcompzilla.la
component_DATA = $(IDL_XPT_FILES)


libcompzilla_la_LDFLAGS = -avoid-version -module
libcompzilla_la_LIBADD = $(XDAMAGE_LIBS)

libcompzilla_la_CPPFLAGS =			\
	$(AM_CPPFLAGS)				\
	$(XDAMAGE_CFLAGS)			\
	-I$(srcdir)/public

# The include of $(GECKO_INCLUDEDIR) in the FULL_GECKO case is for 
# Ubuntu, which has a mix of the gecko-sdk directory arrangement
# with including the full set of headers
if HAVE_FULL_GECKO
libcompzilla_la_CPPFLAGS +=			\
	-I$(GECKO_INCLUDEDIR)			\
	-I$(GECKO_INCLUDEDIR)/nspr		\
	-I$(GECKO_INCLUDEDIR)/caps		\
	-I$(GECKO_INCLUDEDIR)/docshell		\
	-I$(GECKO_INCLUDEDIR)/dom		\
	-I$(GECKO_INCLUDEDIR)/gfx		\
	-I$(GECKO_INCLUDEDIR)/js		\
	-I$(GECKO_INCLUDEDIR)/necko		\
	-I$(GECKO_INCLUDEDIR)/string		\
	-I$(GECKO_INCLUDEDIR)/widget		\
	-I$(GECKO_INCLUDEDIR)/xpcom		\
	-I$(GECKO_INCLUDEDIR)/xpconnect
else
libcompzilla_la_CPPFLAGS +=			\
	-I$(GECKO_INCLUDEDIR)			\
	-I$(GECKO_PRIVATE_INCLUDEDIR)
endif

nodist_libcompzilla_la_SOURCES=			\
	$(IDL_HEADERS)				\
	$(srcdir)/src/compzillaExtension.cpp	\
	$(srcdir)/src/compzillaExtension.h	\
	$(srcdir)/src/compzillaModule.cpp	\
	$(srcdir)/src/compzillaControl.cpp	\
	$(srcdir)/src/compzillaControl.h


#
# xpidl generates .h and .xpt files from .idl files
#

if HAVE_FULL_GECKO
XPIDL_INCLUDES =				\
	-I$(GECKO_IDLDIR)			\
	-I$(srcdir)/public
else
XPIDL_INCLUDES =				\
	-I$(GECKO_IDLDIR)			\
	-I$(srcdir)/public
endif

%.h: %.idl
	$(XPIDL) $(XPIDL_INCLUDES) -m header -e $@ $<

%.xpt: %.idl
	$(XPIDL) $(XPIDL_INCLUDES)  -m typelib -e $@ $<
