#-------------------------------------------------
#
# Project created by QtCreator 2019-03-10T18:12:55
#
#-------------------------------------------------

QT       += core gui widgets concurrent
TARGET = mediaplayer_spivak
TEMPLATE = lib
CONFIG += dll c++11

DEFINES += LIBMEDIAPLAYER_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    mediaplayer_gstreamer.cpp \
    mediaplayer_factory.cpp

HEADERS += \
    mediaplayer_gstreamer.h \
    mediaplayer_factory.h \
    interface_mediaplayer.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

mac: {
    INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
    LIBS += -L/Library/Frameworks/GStreamer.framework/Libraries
}

unix:!mac:{
   CONFIG += link_pkgconfig
   PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0
} else: {
    LIBS += -lgstapp-1.0 -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0
}
