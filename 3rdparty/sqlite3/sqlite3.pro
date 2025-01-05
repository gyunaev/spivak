TARGET = sqlite3
CONFIG = warn_on staticlib
TEMPLATE = lib
DEFINES += SQLITE_OMIT_LOAD_EXTENSION
SOURCES += sqlite3.c
