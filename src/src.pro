#-------------------------------------------------
#
# Project created by QtCreator 2015-11-07T23:19:59
#
#-------------------------------------------------

QT       += core gui widgets network concurrent
#CONFIG += GOOGLEBREAKPAD


TARGET = spivak
TEMPLATE = app

SOURCES += main.cpp\
    mainwindow.cpp \
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
    feedbackdialog.cpp \
    mediaplayer.cpp \
    midisyntheser.cpp \
    midistripper.cpp \
    pluginmanager.cpp \
    notification.cpp \
    messageboxautoclose.cpp \
    collectionentry.cpp \
    collectionprovider.cpp \
    collectionproviderfs.cpp \
    collectionproviderhttp.cpp \
    songqueueitem.cpp \
    songqueueitemretriever.cpp

HEADERS  += mainwindow.h \
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
    feedbackdialog.h \
    crashhandler.h \
    mediaplayer.h \
    midisyntheser.h \
    midistripper.h \
    pluginmanager.h \
    interface_languagedetector.h \
    interface_mediaplayer.h \
    notification.h \
    messageboxautoclose.h \
    collectionentry.h \
    collectionprovider.h \
    collectionproviderfs.h \
    collectionproviderhttp.h \
    songqueueitem.h \
    songqueueitemretriever.h

FORMS    += mainwindow.ui \
    playerwidget.ui \
    dialog_about.ui \
    settingsdialog.ui \
    queuekaraokewidget.ui \
    queuekaraokewidget_addeditdialog.ui \
    queuemusicwidget.ui \
    multimediatestwidget.ui \
    welcome_wizard.ui \
    feedbackdialog.ui

RESOURCES += resources.qrc
DEFINES += SQLITE_OMIT_LOAD_EXTENSION

GOOGLEBREAKPAD {
  SRC += crashhandler.cpp
  LIBS += -L$$PWD/../extralibs/google-breakpad/lib -lgoogle-breakpad
  INCLUDEPATH += $$PWD/../extralibs/google-breakpad/include/
  mac: LIBS += /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation /System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices
  DEFINES += USE_BREAKPAD
}

INCLUDEPATH += $$PWD/.. $$PWD/../extralibs/include
DEPENDPATH += $$PWD/../libkaraokelyrics


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libkaraokelyrics/release/ -lkaraokelyrics -L$$OUT_PWD/../libsonivox/src/release/ -lsonivox
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libkaraokelyrics/debug/ -lkaraokelyrics -L$$OUT_PWD/../libsonivox/src/debug/ -lsonivox
else:unix: LIBS += -L$$OUT_PWD/../libkaraokelyrics/ -lkaraokelyrics -L$$OUT_PWD/../libsonivox/src/ -lsonivox

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libkaraokelyrics/release/libkaraokelyrics.a $$OUT_PWD/../libsonivox/src/release/libsonivox.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libkaraokelyrics/debug/libkaraokelyrics.a $$OUT_PWD/../libsonivox/src/debug/libsonivox.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libkaraokelyrics/release/karaokelyrics.lib $$OUT_PWD/../libsonivox/src/release/sonivox.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libkaraokelyrics/debug/karaokelyrics.lib $$OUT_PWD/../libsonivox/src/debug/sonivox.lib
else:unix: {
    LIBS += -L$$PWD/../extralibs/lib -lsonivox
    PRE_TARGETDEPS += $$OUT_PWD/../libkaraokelyrics/libkaraokelyrics.a $$OUT_PWD/../libsonivox/src/libsonivox.a
}

mac: {
    INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
    LIBS += -L/Library/Frameworks/GStreamer.framework/Libraries
    LIBS += -L$$PWD/../extralibs/lib -lsonivox
}

unix:!mac:{
   CONFIG += link_pkgconfig
   PKGCONFIG += sqlite3 libzip uchardet gstreamer-1.0 gstreamer-app-1.0
} else: {
    LIBS += -lzip -lsqlite3 -luchardet -lgstapp-1.0 -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0
}

win32:!win32-g++: QMAKE_CXXFLAGS+=/Zi
win32:!win32-g++: QMAKE_LFLAGS+= /INCREMENTAL:NO /Debug
win32:RC_ICONS += images/application.ico
