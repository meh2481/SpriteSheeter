#-------------------------------------------------
#
# Project created by QtCreator 2015-06-30T21:00:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SpriteSheeter
TEMPLATE = app


SOURCES += main.cpp\
    SheetEditorView.cpp \
    ZoomableGraphicsView.cpp \
    MainWindow.cpp \
    ImportDialog.cpp \
    IconExportDialog.cpp \
    BalanceSheetDialog.cpp \
    RecentDocuments.cpp \
    BatchRenderer.cpp \
    AnimPreview.cpp \
    Animation.cpp

HEADERS  += \
		FreeImage.h \
    ZoomableGraphicsView.h \
    SheetEditorView.h \
    MainWindow.h \
    ImportDialog.h \
    IconExportDialog.h \
    BalanceSheetDialog.h \
    AnimPreview.h \
    RecentDocuments.h \
    BatchRenderer.h \
    Animation.h

FORMS    += \
    BalanceSheetDialog.ui \
    IconExportDialog.ui \
    ImportDialog.ui \
    MainWindow.ui
	
RC_FILE = spritesheetertest1.rc

RESOURCES = spritesheetertest1.qrc

DISTFILES +=

win32:LIBS += -L"$$_PRO_FILE_PWD_/lib/" -lFreeImage