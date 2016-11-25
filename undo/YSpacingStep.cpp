#include "YSpacingStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

YSpacingStep::YSpacingStep(MainWindow* w, int originalSpacing, int currentSpacing) : UndoStep(w)
{
    orig = originalSpacing;
    current = currentSpacing;
}

void YSpacingStep::undo()
{
    mainWindow->getUI()->ySpacingBox->setValue(orig);
    mainWindow->getSheet()->setYSpacing(orig);
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}

void YSpacingStep::redo()
{
    mainWindow->getUI()->ySpacingBox->setValue(current);
    mainWindow->getSheet()->setYSpacing(current);
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}
