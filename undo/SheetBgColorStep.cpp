#include "SheetBgColorStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QPixmap>
#include <QIcon>

SheetBgColorStep::SheetBgColorStep(MainWindow* w, QColor orig, QColor newC) : UndoStep(w)
{
    origColor = orig;
    newColor = newC;
}

void SheetBgColorStep::undo()
{
    QPixmap colIcon(32, 32);
    colIcon.fill(origColor);
    QIcon ic(colIcon);
    mainWindow->getUI()->sheetBgColSelect->setIcon(ic);
    mainWindow->getSheet()->setBgCol(origColor);
}

void SheetBgColorStep::redo()
{
    QPixmap colIcon(32, 32);
    colIcon.fill(newColor);
    QIcon ic(colIcon);
    mainWindow->getUI()->sheetBgColSelect->setIcon(ic);
    mainWindow->getSheet()->setBgCol(newColor);
}
