include(../common.pri)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

qt_mustache-uselib:!qt_mustache-buildlib {
    LIBS += -L$$QT_MUSTACHE_LIBDIR -l$$QT_MUSTACHE_LIBNAME
} else {
    HEADERS += \
            $$PWD/mustache.h

    SOURCES += \
            $$PWD/mustache.cpp
}

win32 {
    qt_mustache-buildlib:shared:DEFINES += QT_MUSTACHE_EXPORT
    else:qt_mustache-uselib:DEFINES += QT_MUSTACHE_IMPORT
}
