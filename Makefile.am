ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-appdata-check \
	--enable-flac \
	--enable-id3v23 \
	--enable-ogg \
	--enable-opus \
	--enable-man \
	--enable-mp3 \
	--enable-mp4 \
	--enable-nautilus-actions \
	--enable-speex \
	--enable-tests \
	--enable-wavpack
 
SUBDIRS = help po

bin_PROGRAMS = easytag

localedir = $(prefix)/$(DATADIRNAME)/locale
easytag_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(DEPRECATED_CPPFLAGS) \
	-DLOCALEDIR=\"$(localedir)\"

easytag_CFLAGS = \
	$(WARN_CFLAGS) \
	$(EASYTAG_CFLAGS)

easytag_CXXFLAGS = \
	$(WARN_CXXFLAGS) \
	$(EASYTAG_CFLAGS)

easytag_SOURCES = \
	src/id3lib/c_wrapper.cpp \
	src/libapetag/apetaglib.c \
	src/libapetag/is_tag.c \
	src/libapetag/info_mac.c \
	src/libapetag/info_mpc.c \
	src/about.c \
	src/ape_tag.c \
	src/application.c \
	src/bar.c \
	src/browser.c \
	src/browser.h \
	src/cddb.c \
	src/charset.c \
	src/crc32.c \
	src/dlm.c \
	src/easytag.c \
	src/et_core.c \
	src/flac_header.c \
	src/flac_tag.c \
	src/gio_wrapper.cc \
	src/gtk2_compat.c \
	src/id3_tag.c \
	src/id3v24_tag.c \
	src/log.c \
	src/misc.c \
	src/monkeyaudio_header.c \
	src/mpeg_header.c \
	src/mp4_tag.cc \
	src/musepack_header.c \
	src/ogg_header.c \
	src/ogg_tag.c \
	src/opus_header.c \
	src/opus_tag.c \
	src/picture.c \
	src/prefs.c \
	src/scan.c \
	src/scan_dialog.c \
	src/setting.c \
	src/vcedit.c \
	src/wavpack_header.c \
	src/wavpack_tag.c \
	src/win32/win32dep.c

easytag_headers = \
	src/id3lib/id3_bugfix.h \
	src/libapetag/apetaglib.h \
	src/libapetag/is_tag.h \
	src/libapetag/info_mac.h \
	src/libapetag/info_mpc.h \
	src/about.h \
	src/ape_tag.h \
	src/application.h \
	src/bar.h \
	src/cddb.h \
	src/charset.h \
	src/crc32.h \
	src/dlm.h \
	src/easytag.h \
	src/et_core.h \
	src/flac_header.h \
	src/flac_tag.h \
	src/genres.h \
	src/gio_wrapper.h \
	src/gtk2_compat.h \
	src/id3_tag.h \
	src/log.h \
	src/misc.h \
	src/monkeyaudio_header.h \
	src/mpeg_header.h \
	src/mp4_header.h \
	src/mp4_tag.h \
	src/musepack_header.h \
	src/ogg_header.h \
	src/ogg_tag.h \
	src/opus_header.h \
	src/opus_tag.h \
	src/picture.h \
	src/prefs.h \
	src/scan.h \
	src/scan_dialog.h \
	src/setting.h \
	src/ui_manager.h \
	src/vcedit.h \
	src/wavpack_header.h \
	src/wavpack_tag.h \
	src/win32/win32dep.h

easytag_LDADD = \
	$(EASYTAG_LIBS) \
	$(ID3LIB_LIBS) \
	$(easytag_rc)

if WINDOWS_HOST
ICONDIR = $(srcdir)/data/icons/win32
MANIFESTDIR = $(srcdir)/src/win32

easytag_rc.$(OBJEXT): src/win32/easytag.rc src/win32/easytag.manifest data/icons/win32/easytag.ico
	$(AM_V_GEN)$(WINDRES) -DICONDIR='\"$(ICONDIR)\"' \
		-DMANIFESTDIR='\"$(MANIFESTDIR)\"' -i $< -o $@

easytag_rc = easytag_rc.$(OBJEXT)

easytag_LDFLAGS = \
	-Wl,--subsystem,windows

easytag-$(PACKAGE_VERSION)-setup.exe: easytag-win32-installer.nsi easytag.exe Makefile
	$(AM_V_GEN) DESTDIR=`mktemp -d` && \
		make -C $(top_builddir) install DESTDIR=$$DESTDIR >/dev/null && \
		makensis -NOCD -DDESTDIR=$$DESTDIR $< >/dev/null && \
		rm -rf $$DESTDIR
endif

# NSIS installer script.
# TODO: Run script from dist dir.
easytag-win32-installer.nsi: easytag-win32-installer.nsi.in Makefile
	$(AM_V_GEN)$(SED) -e "s|\@PACKAGE_NAME\@|$(PACKAGE_NAME)|" \
		-e "s|\@PACKAGE_TARNAME\@|$(PACKAGE_TARNAME)|" \
		-e "s|\@PACKAGE_URL\@|$(PACKAGE_URL)|" \
		-e "s|\@PACKAGE_VERSION\@|$(PACKAGE_VERSION)|" \
		-e "s|\@PREFIX\@|$(prefix)|" $< > $@

if ENABLE_NAUTILUS_ACTIONS
nautilus_extensiondir = $(NAUTILUS_EXTENSION_DIR)

nautilus_extension_LTLIBRARIES = \
	libnautilus-easytag.la

libnautilus_easytag_la_SOURCES = \
	nautilus-extension/easytag-module.c \
	nautilus-extension/nautilus-easytag.c \
	nautilus-extension/nautilus-easytag.h

libnautilus_easytag_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(DEPRECATED_CPPFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	$(NAUTILUS_CFLAGS)

libnautilus_easytag_la_LDFLAGS = \
	-module -avoid-version -no-undefined

libnautilus_easytag_la_LIBADD = \
	$(NAUTILUS_LIBS)
endif

# Create a directory stamp in the build dir. Used to workaround the case where
# a pattern rule is used for a target with a path that does not ordinarily
# exist in the build dir.
%/.dstamp:
	$(AM_V_at)$(MKDIR_P) $(@D)
	$(AM_V_at)touch $@

@APPDATA_XML_RULES@
@INTLTOOL_XML_RULE@
appdata_in_files = data/easytag.appdata.xml.in
appdata_XML = $(appdata_in_files:.appdata.xml.in=.appdata.xml)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = data/easytag.desktop.in
$(desktop_in_files:.desktop.in=.desktop): data/.dstamp
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

if ENABLE_MAN
doc/easytag.1: doc/easytag.xml Makefile
	$(AM_V_GEN)$(XSLTPROC) --nonet --param man.charmap.use.subset 0 \
		--path $(builddir)/doc --output $(builddir)/doc/ \
		http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

nodist_man_MANS = doc/easytag.1
endif

pixmaps = \
	data/pixmaps/all_downcase.xpm \
	data/pixmaps/all_uppercase.xpm \
	data/pixmaps/artist.xpm \
	data/pixmaps/artist_album.xpm \
	data/pixmaps/first_letter_uppercase.xpm \
	data/pixmaps/first_letter_uppercase_word.xpm \
	data/pixmaps/freedb.xpm \
	data/pixmaps/gnudb.xpm \
	data/pixmaps/invert_selection.xpm \
	data/pixmaps/mask.xpm \
	data/pixmaps/musicbrainz.xpm \
	data/pixmaps/red_lines.xpm \
	data/pixmaps/sequence_track.xpm \
	data/pixmaps/unselect_all.xpm

iconthemedir = $(datadir)/icons/hicolor

appicon16dir = $(iconthemedir)/16x16/apps
appicon22dir = $(iconthemedir)/22x22/apps
appicon24dir = $(iconthemedir)/24x24/apps
appicon32dir = $(iconthemedir)/32x32/apps
appicon48dir = $(iconthemedir)/48x48/apps
appicon64dir = $(iconthemedir)/64x64/apps
appicon128dir = $(iconthemedir)/128x128/apps
appicon256dir = $(iconthemedir)/256x256/apps
appiconscalabledir = $(iconthemedir)/scalable/apps

dist_appicon16_DATA = \
	data/icons/16x16/easytag.png
dist_appicon22_DATA = \
	data/icons/22x22/easytag.png
dist_appicon24_DATA = \
	data/icons/24x24/easytag.png
dist_appicon32_DATA = \
	data/icons/32x32/easytag.png
dist_appicon48_DATA = \
	data/icons/48x48/easytag.png
dist_appicon64_DATA = \
	data/icons/64x64/easytag.png
dist_appicon128_DATA = \
	data/icons/128x128/easytag.png
dist_appicon256_DATA = \
	data/icons/256x256/easytag.png
dist_appiconscalable_DATA = \
	data/icons/scalable/easytag.svg

UPDATE_ICON_CACHE = gtk-update-icon-cache --ignore-theme-index --force

install-data-hook: install-update-icon-cache
uninstall-hook: uninstall-update-icon-cache

install-update-icon-cache:
	$(AM_V_at)$(POST_INSTALL)
	test -n "$(DESTDIR)" || $(UPDATE_ICON_CACHE) "$(iconthemedir)"

uninstall-update-icon-cache:
	$(AM_V_at)$(POST_UNINSTALL)
	test -n "$(DESTDIR)" || $(UPDATE_ICON_CACHE) "$(iconthemedir)"

dist_noinst_DATA = \
	$(appdata_in_files) \
	$(desktop_in_files) \
	$(easytag_headers) \
	src/id3lib/patch_id3lib_3.8.3_UTF16_writing_bug.diff \
	src/libapetag/COPYING.LGPL \
	src/libapetag/README.apetag \
	src/libapetag/id3v2_read.c \
	src/libapetag/id3v2_read.h \
	src/mp4_header.cc \
	src/win32/resource.h \
	data/icons/win32/easytag.ico \
	data/nsis/easytag-header.bmp \
	data/nsis/easytag-sidebar.bmp \
	$(pixmaps) \
	doc/easytag.xml \
	COPYING.GPL3 \
	easytag-win32-installer.nsi.in \
	HACKING

dist_noinst_SCRIPTS = \
	autogen.sh

if EASYTAG_ENABLE_TESTS
tests/test-desktop-file-validate.sh: Makefile tests/.dstamp
	$(AM_V_GEN)echo "#!/bin/sh -e" > $@; \
		echo "$(DESKTOP_FILE_VALIDATE) $(top_builddir)/$(desktop_DATA) || exit 1" >> $@; \
		chmod +x $@

# Not automake built-in TESTS_ENVIRONMENT!
TEST_ENVIRONMENT = MALLOC_CHECK_=2 MALLOC_PERTURB_=$$(($${RANDOM:-256} % 256))

# test: run all tests.
test: $(check_PROGRAMS)
	$(AM_V_at)$(TEST_ENVIRONMENT) $(GTESTER) --verbose $(check_PROGRAMS)

# test-report: run tests and generate report.
# perf-report: run tests with -m perf and generate report.
# full-report: like test-report: with -m perf and -m slow.
test-report perf-report full-report: $(check_PROGRAMS)
	$(AM_V_at)test -z "$(check_PROGRAMS)" || { \
	  case $@ in \
	  test-report) test_options="-k";; \
	  perf-report) test_options="-k -m=perf";; \
	  full-report) test_options="-k -m=perf -m=slow";; \
	  esac ; \
	  $(TEST_ENVIRONMENT) $(GTESTER) --verbose $$test_options -o test-log.xml $(check_PROGRAMS) ; \
	  if test -d "$(top_srcdir)/.git" ; then \
	    REVISION=`git describe` ; \
	  else \
	    REVISION=$(PACKAGE_VERSION) ; \
	  fi ; \
	  echo '<?xml version="1.0"?>' > $@.xml ; \
	  echo '<report-collection>' >> $@.xml ; \
	  echo '<info>' >> $@.xml ; \
	  echo '  <package>$(PACKAGE_NAME)</package>' >> $@.xml ; \
	  echo '  <version>$(PACKAGE_VERSION)</version>' >> $@.xml ; \
	  echo "  <revision>$$REVISION</revision>" >> $@.xml ; \
	  echo '</info>' >> $@.xml ; \
	  sed '1,1s/^<?xml\b[^>?]*?>//' < test-log.xml >> $@.xml ; \
	  rm test-log.xml ; \
	  echo >> $@.xml ; \
	  echo '</report-collection>' >> $@.xml ; \
	  $(GTESTER_REPORT) --version 2>/dev/null 1>&2 ; test "$$?" != 0 || $(GTESTER_REPORT) $@.xml >$@.html ; \
	}

check_PROGRAMS = \
	tests/test-scan

tests_test_scan_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_builddir) \
	$(DEPRECATED_CPPFLAGS)

tests_test_scan_CFLAGS = \
	$(WARN_CFLAGS) \
	$(EASYTAG_CFLAGS)

tests_test_scan_SOURCES = \
	tests/test-scan.c \
	src/scan.c

tests_test_scan_LDADD = \
	$(EASYTAG_LIBS)

check_SCRIPTS = \
	tests/test-desktop-file-validate.sh

TESTS = \
	$(check_PROGRAMS) \
	$(check_SCRIPTS)
endif

CLEANFILES = \
	$(appdata_XML) \
	$(check_SCRIPTS) \
	$(desktop_DATA) \
	$(easytag_rc) \
	$(nodist_man_MANS) \
	*.log \
	easytag-$(PACKAGE_VERSION)-setup.exe \
	easytag-win32-installer.nsi

DISTCLEANFILES = \
	po/.intltool-merge-cache

check-local: test
clean-local: clean-local-dstamp
clean-local-dstamp:
	-rm -f data/.dstamp
	-rm -f tests/.dstamp

git-changelog-hook:
	$(AM_V_at)if $(top_srcdir)/build-aux/missing --run git \
		--git-dir=$(top_srcdir)/.git --work-tree=$(top_srcdir) log \
		--stat -M -C --name-status --no-color \
		| fmt --split-only >.ChangeLog.tmp; \
	then mv -f .ChangeLog.tmp "$(top_distdir)/ChangeLog"; \
	else rm -f .ChangeLog.tmp; exit 1; fi

dist-hook: git-changelog-hook

.PHONY: clean-local-dstamp git-changelog-hook
.PHONY: test test-report perf-report full-report
