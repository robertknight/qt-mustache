TEMPLATE=subdirs
CONFIG += ordered
include(common.pri)
qt_mustache-uselib:SUBDIRS=buildlib
SUBDIRS+=tests
