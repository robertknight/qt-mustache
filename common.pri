exists(config.pri):infile(config.pri, QT_MUSTACHE_LIBRARY, yes): CONFIG += qt_mustache-uselib
TEMPLATE += fakelib
QT_MUSTACHE_LIBNAME = qt-mustache
CONFIG(debug, debug|release) {
    mac:QT_MUSTACHE_LIBNAME = $$member(QT_MUSTACHE_LIBNAME, 0)_debug
    else:win32:QT_MUSTACHE_LIBNAME = $$member(QT_MUSTACHE_LIBNAME, 0)d
}
TEMPLATE -= fakelib
QT_MUSTACHE_LIBDIR = $$PWD/lib
unix:qt_mustache-uselib:!qt_mustache-buildlib:QMAKE_RPATHDIR += $$QT_MUSTACHE_LIBDIR
