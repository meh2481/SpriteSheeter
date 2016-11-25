#include "SheetFontStep.h"
#include "MainWindow.h"

SheetFontStep::SheetFontStep(MainWindow* w, QFont before, QFont after) : UndoStep(w)
{
    origFont = before;
    newFont = after;
}

void SheetFontStep::undo()
{
    mainWindow->getSheet()->setFont(origFont);
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}

void SheetFontStep::redo()
{
    mainWindow->getSheet()->setFont(newFont);
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}
