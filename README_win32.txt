This is a test version of the win32 port of EasyTAG 1.99.9 (http://easytag.sourceforge.net/)


I. BINARIES INSTALL
===================

You'll need to install GTK+ runtime to run EasyTAG.
Get it at http://sourceforge.net/project/showfiles.php?group_id=121075&package_id=132255
The GTK+ runtime will add the directory "C:\Program Files\Common files\GTK\2.0\bin" in your PATH environment variable.

You can run from any directory. It should autodetect GTK libs location and store preferences in the localized version of
the "C:\Documents and Settings\<user>\Application Data\.easytag" folder

It should autodetect your audio player. foobar2k and winamp in order, if you have them installed. If not you can set 
your audio player in the settings dialog.

It should use the langage of your Windows installation. It can be overriden by setting the EASYTAGLANG environment variable
to the two letter country code found in the "locale" subdirectory. 
To define this variable, go in the "Advanced" tab of the "Desktop" properties window. Click the  "Environment variable"
button, and then create it with the "New" button of the User variables frame.
Example for french:
    set EASYTAGLANG=fr
    
Crash at start:
---------------
    If the program crashes immediatelly at start without any message, it may be due to the locale which isn't detected 
correctly.
In this case, try to declare the EASYTAGLANG variable using your right locale as value, else with "en".



II. DIFFERENCE WITH UNIX VERSION
================================

- do not scan default directory at startup (less confusing on first launch)
- "Show hidden directory..." and corresponding setting check box removed (makes no sense on win32)
- don't check if audio executable exist because it can contains spaces and the current algorithm do not work in that case


III. ISSUES
===========

- info displayed for ape files is wrong


IV. TODO
========

- crash handler using unix signals to replace with something else
- minimize main window should also close/minimize the scanner window.
- add detection of windows media player location if can't find other players
- go to home directory should go to c:\documents and settings\users\my documents\my music (detect the localized directory)


V. TO COMPILE
=============

- install MinGW with gcc 3.4.4 and g++ (important as gcc 3.4.2 in current has a serious bug with paths)
- install MSYS
- install gtk+ development and runtime package and their dependencies in the MinGW directory (see http://www.gimp.org/~tml/gimp/win32/downloads.html)
	* libiconv-1.9.1.bin.woe32.zip
	* gettext-0.14.5.zip
	* gettext-dev-0.14.5.zip
	* libtool-1.5.8-bin.zip
	* libtool-1.5.8-lib.zip
	
	* mingw-utils-0.3.tar.gz (for debugger "drmingw", see infos in doc/ for installation:
	                          - unzip in MinGW/ directory
	                          - edit drmingw.reg to set the correct path to drmingw.exe and apply it
	                          - run "drmingw -i")
    * (interesting informations on http://developer.pidgin.im/wiki/BuildingWinPidgin)
    
- install pkg-config (see http://www.gimp.org/~tml/gimp/win32/downloads.html)

- add in /etc/profile:
export PKG_CONFIG_PATH=/mingw/lib/pkgconfig:/usr/local/lib/pkgconfig

- relaunch MSYS and check that pkg-config works:
pkg-config --list-all
    => should list gtk and its dependencies


1) id3lib-3.8.3 :
-----------------

- edit configure.in : comment check for truncate() (line 252 to 255)
- aclocal && automake && autoconf
- CXXFLAGS="-DID3LIB_LINKOPTION=1" ./configure && make install

2) libid3tag-0.15.1b :
----------------------

- install (see: http://sourceforge.net/project/showfiles.php?group_id=23617&package_id=16861&release_id=344693)
    * zlib-1.2.3-bin.zip
    * zlib-1.2.3-lib.zip

- ./configure && make install


3) libmp4v2 (in mpeg4ip) :
--------------------------

- compile and install SDL (SDL-1.2.9.tar.gz : ./configure && make install)
- ./bootstrap && ./configure
- go to ./lib/mp4v2
- make install
- go to ./include
- cp *.h /usr/local


4) libogg-1.1.3 :
-----------------

- ./configure && make install


5) libvorbis-1.2.0 :
--------------------

- ./configure

- compile will fail when linking libvorbisfile with a libogg missing dependency (libtool bug)
to fix the problem:

libvorbis-1.1.2, in file "lib/Makefile" line 252, replace:
libvorbis-1.2.0, in file "lib/Makefile" line 267, replace:
    libvorbisfile_la_LIBADD = libvorbis.la
by
    libvorbisfile_la_LIBADD = libvorbis.la -L/local/lib -logg

- make install


6) speex-1.2beta2 :
-------------------

./configure && make install


7) flac-1.1.3 :
---------------

./configure && make install


8) wavpack-4.41.0 :
-------------------

- wavpack-4.40.0, if compile fails, replace in the file 'cli\utils.c', line 304 : 
    return (int)(pos - cp) & 1;
  by :
    return (int)((int)pos - (int)cp) & 1;

./configure && make install


9) easytag :
------------

make -f Makefile.mingw install


all files are in win32-install-dir
