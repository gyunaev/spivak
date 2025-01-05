!greaterThan(QT_MAJOR_VERSION, 4) {
   error(Spivak will not build with Qt4)
}
!greaterThan(QT_MAJOR_VERSION, 5) {
   warning(Spivak generally requires Qt6 but Qt $$[QT_VERSION] was detected. Your build may fail. Make sure you invoke proper qmake i.e. qmake6 instead of qmake)
}

SUBDIRS += 3rdparty libkaraokelyrics src
TEMPLATE = subdirs
libkaraokelyrics.depends = 3rdparty
src.depends = libkaraokelyrics
TRANSLATIONS = i18n/spivak_en_US.ts i18n/spivak_sk.ts

INSTALLS += executable executable  desktopfile icon

executable.path = /$(DESTDIR)/usr/bin
executable.files = src/spivak

icon.path = /$(DESTDIR)/usr/share/icons/hicolor/64x64/apps
icon.files = packaging/spivak.png

desktopfile.path = /$(DESTDIR)/usr/share/applications
desktopfile.files = packaging/spivak.desktop
