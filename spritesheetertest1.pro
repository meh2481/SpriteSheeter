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
    Animation.cpp \
    Sheet.cpp \
    Frame.cpp \
    undo/UndoStep.cpp \
    undo/FontColorStep.cpp \
    undo/FrameBgColorStep.cpp \
    undo/SheetBgColorStep.cpp \
    undo/SheetBgTransparentStep.cpp \
    undo/FrameBgTransparentStep.cpp \
    undo/SheetFontStep.cpp \
    undo/YSpacingStep.cpp \
    undo/XSpacingStep.cpp \
    undo/MinimizeWidthCheckboxStep.cpp \
    undo/SheetWidthStep.cpp \
    undo/ReverseAnimStep.cpp \
    undo/AnimNameStep.cpp \
    undo/NameVisibleStep.cpp \
    undo/BalanceAnimStep.cpp \
    undo/RemoveDuplicateStep.cpp \
    undo/DragDropStep.cpp

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
    Animation.h \
    Sheet.h \
    BalancePos.h \
    Frame.h \
    undo/UndoStep.h \
    undo/FontColorStep.h \
    undo/FrameBgColorStep.h \
    undo/SheetBgColorStep.h \
    undo/SheetBgTransparentStep.h \
    undo/FrameBgTransparentStep.h \
    undo/SheetFontStep.h \
    undo/YSpacingStep.h \
    undo/XSpacingStep.h \
    undo/MinimizeWidthCheckboxStep.h \
    undo/SheetWidthStep.h \
    undo/ReverseAnimStep.h \
    undo/AnimNameStep.h \
    undo/NameVisibleStep.h \
    undo/BalanceAnimStep.h \
    undo/RemoveDuplicateStep.h \
    undo/DragDropStep.h

FORMS    += \
    BalanceSheetDialog.ui \
    IconExportDialog.ui \
    ImportDialog.ui \
    MainWindow.ui
	
RC_FILE = spritesheetertest1.rc

RESOURCES = spritesheetertest1.qrc

DISTFILES +=

win32:LIBS += -L"$$_PRO_FILE_PWD_/lib/" -lFreeImage