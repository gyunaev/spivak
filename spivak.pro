!greaterThan(QT_MAJOR_VERSION, 4) {
   error(Spivak will not build with Qt4)
}
!greaterThan(QT_MAJOR_VERSION, 5) {
   warning(Spivak generally requires Qt6 but Qt $$[QT_VERSION] was detected. Your build may fail. Make sure you invoke proper qmake i.e. qmake6 instead of qmake)
}

SUBDIRS += libsonivox libkaraokelyrics src
TEMPLATE = subdirs
src.depends = libkaraokelyrics
TRANSLATIONS = i18n/spivak_en_US.ts i18n/spivak_sk.ts
