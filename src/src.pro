#-------------------------------------------------
#
# Project created by QtCreator 2015-11-07T23:19:59
#
#-------------------------------------------------

# Dump all qmake variables
#for(var, $$list($$enumerate_vars())) {
#    message($$var $$eval($$var))
#}

QT       += core gui widgets network

TARGET = spivak
TEMPLATE = app
CONFIG += c++11

SOURCES += main.cpp\
    languagedetector.cpp \
    mainwindow.cpp \
    mediaplayer.cpp \
    settings.cpp \
    playerlyrics.cpp \
    playerlyricscdg.cpp \
    songqueue.cpp \
    playerlyricstext.cpp \
    playerlyrictext_line.cpp \
    karaokepainter.cpp \
    playerrenderer.cpp \
    logger.cpp \
    karaokewidget.cpp \
    karaokesong.cpp \
    playerwidget.cpp \
    playerbutton.cpp \
    currentstate.cpp \
    actionhandler.cpp \
    actionhandler_lirc.cpp \
    actionhandler_webserver.cpp \
    karaokeplayable.cpp \
    karaokeplayable_file.cpp \
    karaokeplayable_zip.cpp \
    karaokeplayable_kfn.cpp \
    util.cpp \
    songdatabasescanner.cpp \
    background.cpp \
    backgroundcolor.cpp \
    backgroundimage.cpp \
    backgroundvideo.cpp \
    settingsdialog.cpp \
    colorbutton.cpp \
    labelshowhelp.cpp \
    musiccollectionmanager.cpp \
    queuekaraokewidget.cpp \
    queuetableviewmodels.cpp \
    queuekaraokewidget_addeditdialog.cpp \
    queuemusicwidget.cpp \
    eventor.cpp \
    multimediatestwidget.cpp \
    welcome_wizard.cpp \
    database.cpp \
    database_songinfo.cpp \
    database_statement.cpp \
    actionhandler_webserver_socket.cpp \
    midisyntheser.cpp \
    midistripper.cpp \
    notification.cpp \
    messageboxautoclose.cpp \
    collectionentry.cpp \
    collectionprovider.cpp \
    collectionproviderfs.cpp \
    collectionproviderhttp.cpp \
    songqueueitem.cpp \
    songqueueitemretriever.cpp \
    mediaplayerinitializer.cpp

HEADERS  += mainwindow.h \
    languagedetector.h \
    mediaplayer.h \
    settings.h \
    playerlyrics.h \
    playerlyricscdg.h \
    songqueue.h \
    playerlyricstext.h \
    playerlyrictext_line.h \
    karaokepainter.h \
    playerrenderer.h \
    logger.h \
    util.h \
    karaokewidget.h \
    karaokesong.h \
    playerwidget.h \
    playerbutton.h \
    currentstate.h \
    actionhandler.h \
    actionhandler_lirc.h \
    actionhandler_webserver.h \
    version.h \
    karaokeplayable.h \
    karaokeplayable_file.h \
    karaokeplayable_zip.h \
    karaokeplayable_kfn.h \
    aes.h \
    songdatabasescanner.h \
    background.h \
    backgroundcolor.h \
    backgroundimage.h \
    backgroundvideo.h \
    settingsdialog.h \
    colorbutton.h \
    labelshowhelp.h \
    musiccollectionmanager.h \
    queuekaraokewidget.h \
    queuetableviewmodels.h \
    queuekaraokewidget_addeditdialog.h \
    queuemusicwidget.h \
    eventor.h \
    multimediatestwidget.h \
    welcome_wizard.h \
    database.h \
    database_songinfo.h \
    database_statement.h \
    actionhandler_webserver_socket.h \
    midisyntheser.h \
    midistripper.h \
    notification.h \
    messageboxautoclose.h \
    collectionentry.h \
    collectionprovider.h \
    collectionproviderfs.h \
    collectionproviderhttp.h \
    songqueueitem.h \
    songqueueitemretriever.h \
    mediaplayerinitializer.h

FORMS    += mainwindow.ui \
    playerwidget.ui \
    dialog_about.ui \
    registrationdialog.ui \
    settingsdialog.ui \
    queuekaraokewidget.ui \
    queuekaraokewidget_addeditdialog.ui \
    queuemusicwidget.ui \
    multimediatestwidget.ui \
    welcome_wizard.ui

RESOURCES += resources.qrc
DEFINES += SQLITE_OMIT_LOAD_EXTENSION HAVE_LIBCLD2

SRCROOT=$$PWD/../
INCLUDEPATH += $$SRCROOT/3rdparty $$SRCROOT $$SRCROOT/3rdparty/sqlite3
DEPENDPATH += $$SRCROOT/libkaraokelyrics

# Handle libzip and GStreamer dependency with pkgconfig on Linux, specify exact paths on Mac
unix:!mac:{
    CONFIG += link_pkgconfig
    PKGCONFIG += libzip gstreamer-1.0 gstreamer-app-1.0
} else: {
    INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
    LIBS += -L/Library/Frameworks/GStreamer.framework/Libraries -lgstapp-1.0 -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0 -lzip
}

# Windows builds debug/release libs in different directories while other platforms use a single dir
win32: {

    RC_ICONS += images/application.ico
    QMAKE_CXXFLAGS+=/Zi
    QMAKE_LFLAGS+= /INCREMENTAL:NO /Debug
    LIBS += crypt32.lib

    CONFIG(debug, debug|release) {
        LIBSUBDIR="debug"
    } else: {
        LIBSUBDIR="release"
    }
} else: {
    LIBSUBDIR=""
}

BUILDROOT=$$OUT_PWD/../
LIBS += -L$$BUILDROOT/3rdparty/cld2 -L$$BUILDROOT/3rdparty/libsonivox/src/$$LIBSUBDIR -L$$BUILDROOT/3rdparty/uchardet/$$LIBSUBDIR -L$$BUILDROOT/3rdparty/sqlite3 -L$$BUILDROOT/libkaraokelyrics/$$LIBSUBDIR
LIBS += -lsqlite3 -luchardet -lcld2 -lsonivox -lkaraokelyrics
