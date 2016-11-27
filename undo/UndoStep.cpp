#include "UndoStep.h"

UndoStep::UndoStep(MainWindow* window)
{
    mainWindow = window;
    saved = false;
}
