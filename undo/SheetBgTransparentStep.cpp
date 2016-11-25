#include "SheetBgTransparentStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

SheetBgTransparentStep::SheetBgTransparentStep(MainWindow* w, bool prev, bool next) : UndoStep(w)
{
    previousTransparent = prev;
    newTransparent = next;
}

void SheetBgTransparentStep::undo()
{
    mainWindow->getUI()->sheetBgColSelect->setEnabled(!previousTransparent);
    mainWindow->getSheet()->setBgTransparent(previousTransparent);
    mainWindow->getUI()->sheetBgTransparent->setChecked(previousTransparent);
}

void SheetBgTransparentStep::redo()
{
    mainWindow->getUI()->sheetBgColSelect->setEnabled(!newTransparent);
    mainWindow->getSheet()->setBgTransparent(newTransparent);
    mainWindow->getUI()->sheetBgTransparent->setChecked(newTransparent);
}
