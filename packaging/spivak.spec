Summary: Free and feature-rich Karaoke player
Name: spivak
Version: 2.0
Release: 5
License: GPLv3+
Group: Applications/Multimedia
Packager: gyunaev@ulduzsoft.com
URL: http://www.ulduzsoft.com/spivak
Source0: %{name}-%{version}.tar.gz

Requires: libzip
Requires:  qt6-gui
Requires:  qt6-core
Requires:  qt6-imageformat
Requires:  qt6-widgets
Requires:  qt6-base-common
Requires:  qt6-concurrent
Requires:  qt6-network
Requires:  qt6-base
Requires:  gstreamer-1.0
Requires:  gstreamer-app-1.0

BuildRequires:  pkgconfig
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-app-1.0)

BuildRequires:  qt6-tools-linguist
BuildRequires:  qt6-gui-devel
BuildRequires:  qt6-core-devel
BuildRequires:  qt6-widgets-devel
BuildRequires:  qt6-base-common-devel
BuildRequires:  qt6-tools-linguist
BuildRequires:  qt6-concurrent-devel
BuildRequires:  qt6-network-devel
BuildRequires:  qt6-base-devel
BuildRequires:  libzip-devel

%description
Spivak is a free, cross-platform (Linux/Windows/OS X) Karaoke player 
based on GStreamer and Qt5. It supports a wide range of Karaoke formats, 
with the goal of playing all more or less widespread Karaoke formats on 
all popular platforms. It also has strong support for foreign languages, 
so playing Karaoke in Japanese, Russian or Hindu is fully supported.
 
Author:
-------
George Yunaev
%files
%defattr(-,root,root)
/usr/bin/spivak
%defattr(-,root,root)
/usr/share/applications/spivak.desktop
%defattr(-,root,root)
/usr/share/pixmaps/spivak.png

%prep
%setup

%build
qmake6
%make_build


%install
make install INSTALL_ROOT="%buildroot"
