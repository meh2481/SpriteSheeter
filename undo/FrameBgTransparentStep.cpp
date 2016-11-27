#include "FrameBgTransparentStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

FrameBgTransparentStep::FrameBgTransparentStep(MainWindow* w, bool prev, bool next) : UndoStep(w)
{
    previousTransparent = prev;
    newTransparent = next;
}

void FrameBgTransparentStep::undo()
{
    mainWindow->getUI()->frameBgColSelect->setEnabled(!previousTransparent);
    mainWindow->getSheet()->setFrameBgTransparent(previousTransparent);
    mainWindow->getUI()->frameBgTransparent->setChecked(previousTransparent);
    mainWindow->drawAnimation();
}

void FrameBgTransparentStep::redo()
{
    mainWindow->getUI()->frameBgColSelect->setEnabled(!newTransparent);
    mainWindow->getSheet()->setFrameBgTransparent(newTransparent);
    mainWindow->getUI()->frameBgTransparent->setChecked(newTransparent);
    mainWindow->drawAnimation();
}
