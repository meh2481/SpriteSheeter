#include "FrameBgColorStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QPixmap>
#include <QIcon>

FrameBgColorStep::FrameBgColorStep(MainWindow* w, QColor origC, QColor newC) : UndoStep(w)
{
    origColor = origC;
    newColor = newC;
}

void FrameBgColorStep::undo()
{
    QPixmap colIcon(32, 32);
    colIcon.fill(origColor);
    QIcon ic(colIcon);
    mainWindow->getUI()->frameBgColSelect->setIcon(ic);
    mainWindow->getSheet()->setFrameBgCol(origColor);
    mainWindow->drawAnimation();
}

void FrameBgColorStep::redo()
{
    QPixmap colIcon(32, 32);
    colIcon.fill(newColor);
    QIcon ic(colIcon);
    mainWindow->getUI()->frameBgColSelect->setIcon(ic);
    mainWindow->getSheet()->setFrameBgCol(newColor);
    mainWindow->drawAnimation();
}
