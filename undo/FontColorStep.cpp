#include <QPixmap>
#include <QIcon>
#include "MainWindow.h"
#include "FontColorStep.h"
#include "ui_MainWindow.h"

FontColorStep::FontColorStep(MainWindow* window, QColor const & origC, QColor const & newC) : UndoStep(window)
{
    origColor = origC;
    newColor = newC;
}

void FontColorStep::undo()
{
    QPixmap colIcon(32, 32);
    colIcon.fill(origColor);
    QIcon ic(colIcon);
    mainWindow->getUI()->fontColSelect->setIcon(ic);
    mainWindow->getSheet()->setFontColor(origColor);
}

void FontColorStep::redo()
{
    QPixmap colIcon(32, 32);
    colIcon.fill(newColor);
    QIcon ic(colIcon);
    mainWindow->getUI()->fontColSelect->setIcon(ic);
    mainWindow->getSheet()->setFontColor(newColor);
}
