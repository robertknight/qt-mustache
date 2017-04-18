TEMPLATE = lib
VERSION = 1.0.0
CONFIG += qt dll qt_mustache-buildlib
mac:CONFIG += absolute_library_soname
win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release build_all

include(../src/qt-mustache.pri)

TARGET = $$QT_MUSTACHE_LIBNAME
DESTDIR = $$QT_MUSTACHE_LIBDIR

win32 {
    DLLDESTDIR = $$[QT_INSTALL_BINS]
    QMAKE_DISTCLEAN += $$[QT_INSTALL_BINS]\$${QT_MUSTACHE_LIBNAME}.dll
}

unix {
    isEmpty(PREFIX): PREFIX = /usr/local
    header.files = $$HEADERS
    header.path = $$PREFIX/include
    target.path = $$PREFIX/lib
    INSTALLS += target header
}
