TARGET = testapp
CONFIG += warn_on
CONFIG -= QT
TEMPLATE = app
INCLUDEPATH += ../include
LIBS += -L../src  -lsonivox
SOURCES += test_app.c
