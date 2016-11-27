#include "XSpacingStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

XSpacingStep::XSpacingStep(MainWindow* w, int previousX, int currentX, int previousSheetW) : UndoStep(w)
{
    prevX = previousX;
    curX = currentX;
    prevW = previousSheetW;
}

void XSpacingStep::undo()
{
    mainWindow->getSheet()->setXSpacing(prevX);
    mainWindow->userEditingWidth = false;
    mainWindow->getUI()->sheetWidthBox->setValue(prevW);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->updateSelectedAnim();
}

void XSpacingStep::redo()
{
    mainWindow->getSheet()->setXSpacing(curX);
    int minW = mainWindow->getSheet()->getSmallestPossibleWidth();
    mainWindow->userEditingWidth = false;
    if(minW > mainWindow->getUI()->sheetWidthBox->value())
        mainWindow->getUI()->sheetWidthBox->setValue(minW);
    else
        mainWindow->getUI()->sheetWidthBox->setValue(prevW);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->updateSelectedAnim();
}
