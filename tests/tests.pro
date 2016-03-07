TEMPLATE = app
QT += testlib
QT -= gui
CONFIG -= app_bundle

include(../src/qt-mustache.pri)

!win32 {
  QMAKE_CXXFLAGS += -Werror -Wall -Wextra -Wnon-virtual-dtor
}

# Input
HEADERS += test_mustache.h
SOURCES += test_mustache.cpp

# Copies the given files to the destination directory
defineTest(copyToDestdir) {
    files = $$1

    for(FILE, files) {
        DDIR = $$OUT_PWD

        # Replace slashes in paths with backslashes for Windows
        win32:FILE ~= s,/,\\,g
        win32:DDIR ~= s,/,\\,g

        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$FILE) $$quote($$DDIR) $$escape_expand(\\n\\t)
    }

    export(QMAKE_POST_LINK)
}

copyToDestdir($$PWD/specs/*.json)
