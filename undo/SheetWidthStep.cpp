#include "SheetWidthStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

SheetWidthStep::SheetWidthStep(MainWindow* w, int prevW, int curW) : UndoStep(w)
{
    prev = prevW;
    next = curW;
}

void SheetWidthStep::undo()
{
    mainWindow->getSheet()->setWidth(prev);
    mainWindow->getUI()->sheetWidthBox->setValue(prev);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}

void SheetWidthStep::redo()
{
    mainWindow->getSheet()->setWidth(next);
    mainWindow->getUI()->sheetWidthBox->setValue(next);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}
