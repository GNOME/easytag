noinst_LIBRARIES = libid3bugfix.a

dist_noinst_DATA = \
	patch_id3lib_3.8.3_UTF16_writing_bug.diff

libid3bugfix_a_CPPFLAGS = \
	-I$(top_builddir) \
	$(GTK_CFLAGS) \
	$(WARN_CPPFLAGS) \
	-DLOCALE=\"$(localedir)\"

libid3bugfix_a_SOURCES = \
	c_wrapper.cpp \
	id3_bugfix.h
