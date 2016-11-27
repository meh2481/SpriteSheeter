#include "MinimizeWidthCheckboxStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

MinimizeWidthCheckboxStep::MinimizeWidthCheckboxStep(MainWindow* w, bool prevChecked, bool nextChecked, int lastSheetW) : UndoStep(w)
{
    prev = prevChecked;
    next = nextChecked;
    lastW = lastSheetW;
}

void MinimizeWidthCheckboxStep::undo()
{
    mainWindow->userEditingWidth = false;
    mainWindow->getUI()->sheetWidthBox->setValue(lastW);
    if(prev)
        mainWindow->minimizeSheetWidth();
    mainWindow->getUI()->minWidthCheckbox->setChecked(prev);
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}

void MinimizeWidthCheckboxStep::redo()
{
    if(next)
        mainWindow->minimizeSheetWidth();
    mainWindow->getUI()->minWidthCheckbox->setChecked(next);
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}
