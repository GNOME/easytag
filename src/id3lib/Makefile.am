CFLAGS = @CFLAGS@ @GTK_CFLAGS@

noinst_LIBRARIES = libid3bugfix.a

INCLUDES = -DLOCALE=\"$(localedir)\"

EXTRA_DIST = \
    patch_id3lib_3.8.3_UTF16_writing_bug.diff


libid3bugfix_a_SOURCES = \
    c_wrapper.cpp \
    id3_bugfix.h

