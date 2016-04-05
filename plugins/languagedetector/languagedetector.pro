TARGET = plugin_langdetect
# We use no_plugin_name_prefix to remove lib... prefix on Linux
CONFIG += warn_on plugin no_plugin_name_prefix
QT += core
TEMPLATE = lib
SOURCES += languagedetector.cpp
HEADERS += languagedetector.h
LIBS += -lcld2

LIBS += -L$$PWD/../../extralibs/lib
INCLUDEPATH += $$PWD/../../extralibs/include

mac: {
    CONFIG += plugin
    QMAKE_LFLAGS_PLUGIN -= -dynamiclib
    QMAKE_LFLAGS_PLUGIN += -bundle
    QMAKE_EXTENSION_SHLIB = bundle
}

unix: {
    QMAKE_LFLAGS += -Wl,--no-undefined
}
