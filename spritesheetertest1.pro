#-------------------------------------------------
#
# Project created by QtCreator 2015-06-30T21:00:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = spritesheetertest1
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    importdialog.cpp \
	Graphics_view_zoom.cpp \
    sheeteditorview.cpp

HEADERS  += mainwindow.h \
    importdialog.h \
	Graphics_view_zoom.h \
    sheeteditorview.h \
	animpreview.h

FORMS    += mainwindow.ui \
    importdialog.ui
	
RC_FILE = spritesheetertest1.rc

RESOURCES = spritesheetertest1.qrc
