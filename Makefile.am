## Autotools settings

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	autogen.sh \
	LICENSE \
	README.md \
	sdk/license.txt \
	$(NULL)

noinst_HEADERS = \
	sdk/XPLMDataAccess.h \
	sdk/XPLMDefs.h \
	sdk/XPLMProcessing.h \
	$(NULL)

lib_LTLIBRARIES = \
	xplogd.la \
	$(NULL)

xplogd_la_SOURCES = \
	main.c \
	$(NULL)

xplogd_la_CFLAGS = \
	$(AM_CFLAGS) \
	-fvisibility=hidden \
	-DLIN=1 \
	$(NULL)

xplogd_la_LDFLAGS = \
	-avoid-version \
	-module \
	-shared \
	$(NULL)

xplogd.xpl: xplogd.la
	cp .libs/xplogd.so xplogd.xpl

plugin: xplogd.xpl
	zip $(PACKAGE)-plugin-$(VERSION).zip xplogd.xpl

.PHONY: plugin
