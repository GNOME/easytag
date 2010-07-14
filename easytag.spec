%define    name      easytag
%define    version   2.1.6
%define    release   1
%define    prefix    /usr

Summary:       Tag editor for MP3 and Ogg Vorbis files.
Summary(fr):   Editeur de tags pour fichiers MP3 et Vorbis Ogg.
Name:          %name
Version:       %version
Release:       %release
License:       GPL
URL:           http://easytag.sourceforge.net
Group:         Applications/Multimedia
Source:        http://prdownloads.sourceforge.net/easytag/%{name}-%{version}.tar.bz2
BuildRoot:     /var/tmp/%name-buildroot
Vendor:        Jerome Couderc <easytag@gmail.com>
Distribution:  Any
Packager:      Jerome Couderc <easytag@gmail.com>
Requires:      glib2     >= 2.8.0
Requires:      gtk2      >= 2.12.0
Requires:      id3lib    >= 3.7.12
Requires:      libid3tag >= 0.15
Requires:      libogg    >= 1.0
Requires:      libvorbis >= 1.0
Requires:      flac      >= 1.1.0
Requires:      libmp4v2  >= 1.5.0.1
Requires:      wavpack   >= 4.40
BuildRequires: glib2-devel     >= 2.8.0
BuildRequires: gtk2-devel      >= 2.12.0
BuildRequires: id3lib-devel    >= 3.7.12
BuildRequires: libid3tag-devel >= 0.15
BuildRequires: libogg-devel    >= 1.0
BuildRequires: libvorbis-devel >= 1.0
BuildRequires: flac-devel      >= 1.1.0
BuildRequires: libmp4v2-devel  >= 1.5.0.1
BuildRequires: wavpack-devel   >= 4.40
Prefix:        %prefix

%description
EasyTAG is an utility for viewing and editing tags for MP3, MP2, FLAC, Ogg 
Vorbis, Speex, MP4/AAC, MusePack, Monkey's Audio and WavPack files. Its simple 
and nice GTK2 interface makes tagging easier under GNU/Linux or Windows.

Features:
  o View, edit, write tags of MP3, MP2 files (ID3 tag with pictures), FLAC files
    (FLAC Vorbis tag), Ogg Vorbis and Speex files (Ogg Vorbis tag), MP4/AAC 
    (MP4/AAC tag), MusePack, Monkey's Audio and WavPack files (APE tag),
  o Can edit more tag fields : Title, Artist, Album, Disc Album, Year, Track
    Number, Genre, Comment, Composer, Original Artist/Performer, Copyright, URL
    and Encoder name,
  o Auto tagging: parse filename and directory to complete automatically the
    fields (using masks),
  o Ability to rename files and directories from the tag (using masks) or by
    loading a text file,
  o Process selected files of the selected directory,
  o Ability to browse subdirectories,
  o Recursion for tagging, removing, renaming, saving...,
  o Can set a field (artist, title,...) to all other selected files,
  o Read file header informations (bitrate, time, ...) and display them,
  o Auto completion of the date if a partial is entered,
  o Undo and redo last changes,
  o Ability to process fields of tag and file name (convert letters into
    uppercase, downcase, ...),
  o Ability to open a directory or a file with an external program,
  o Remote and local CDDB support for manual or automatic search,
  o A tree based browser or a view by Artist & Album,
  o A list to select files,
  o A playlist generator window,
  o A file searching window,
  o Simple and explicit interface!,
  o Brazilian Portuguese, Bulgarian, Chinese, Czech, Danish, Dutch, French, 
    German, Greek, Hungarian, Italian, Japanese, Polish, Romanian, Russian, 
    Spanish, Swedish and Ukrainian translation languages,
  o Written in C and uses GTK+ 2 for the GUI.

%package    docs
Summary:    Documentation for using EasyTAG
Group:      Documentation

%description    docs
This package contains the documentation to help you to use EasyTAG and present
 its features.


%prep
%setup

%build
./configure --prefix=%{prefix}
make

%install
rm -rf ${RPM_BUILD_ROOT}
make prefix=${RPM_BUILD_ROOT}%{prefix} install

%post

%postun

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-, root, root)
%doc ChangeLog INSTALL COPYING README THANKS USERS-GUIDE
%prefix/bin/easytag
%prefix/share/applications/easytag.desktop
%prefix/share/pixmaps/*
%prefix/share/man/*/easytag.*
%prefix/share/easytag/*
%prefix/share/locale/*/*/*

%files docs
%defattr(-,root,root,-)
%doc COPYING
%doc doc/*.html
%doc doc/users_guide_*


%changelog
* Thu Oct 11 2007 Jerome Couderc <easytag@gmail.com>
  - Replace "%prefix/man/*/easytag.*" by "%prefix/share/man/*/easytag.*" to
    fix rpmbuild under Fedora 7

* Fri Jun 1 2007 Jerome Couderc <easytag@gmail.com>
  - Updated (Build)Requires entries for libid3tag

* Mon Feb 21 2007 Jerome Couderc <easytag@gmail.com>
  - Updated to generate doc package

* Wed Aug 25 2005 Jerome Couderc <easytag@gmail.com>
  - Updated (Build)Requires entries for mpeg4ip/faad2
  - Updated description to add MP4/AAC support

* Wed Sep 17 2004 Jerome Couderc <j.couderc@ifrance.com>
  - Updated (Build)Requires entries for gtk+ 2.4.0

* Sun Jan 4 2004 Jerome Couderc <j.couderc@ifrance.com>
  - Updated (Build)Requires entries for FLAC support

* Tue Dec 16 2003 Jerome Couderc <j.couderc@ifrance.com>
  - Updated for the file easytag.desktop

* Mon Dec 31 2002 Jerome Couderc <j.couderc@ifrance.com>
  - Added man page

* Fri Aug 9 2002 Jerome Couderc <j.couderc@ifrance.com>
  - Added swith --disable-flac in the build section

* Thu Dec 18 2001 Jerome Couderc <j.couderc@ifrance.com>
  - Updated for (Build)Requires entries

* Thu Sep 22 2001 Jerome Couderc <j.couderc@ifrance.com>
  - Updated for /etc/X11/applnk/Multimedia/easytag.desktop

* Thu Sep 20 2001 Götz Waschk <waschk@linux-mandrake.com> 0.15.1-1
  - Updated for autoconf

* Fri Jun 2 2000 Jerome Couderc <j.couderc@ifrance.com>
  - Updated to include po files into the rpm package

* Fri May 5 2000 Jerome Couderc <j.couderc@ifrance.com>
  - Initial spec file.
