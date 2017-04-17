!greaterThan(QT_MAJOR_VERSION, 4) {
   error(Spivak requires Qt 5.4 or newer but Qt $$[QT_VERSION] was detected. Make sure you invoke proper qmake i.e. qmake-qt5 instead of qmake)
}


SUBDIRS += libsonivox libkaraokelyrics plugins src
TEMPLATE = subdirs
src.depends = libkaraokelyrics
