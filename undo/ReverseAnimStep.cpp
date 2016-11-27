#include "ReverseAnimStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

ReverseAnimStep::ReverseAnimStep(MainWindow* w, int curSelectedAnim) : UndoStep(w)
{
    anim = curSelectedAnim;
    prevW = w->getUI()->sheetWidthBox->value();
}

void ReverseAnimStep::undo()
{
    redo(); //They both are the same
}

void ReverseAnimStep::redo()
{
    Animation* a = mainWindow->getSheet()->getAnimation(anim);
    if(a)
    {
        a->reverse();
        mainWindow->getSheet()->refresh();   //Tell sheet to recalculate positions
        mainWindow->drawAnimation();
        mainWindow->getUI()->sheetWidthBox->setValue(prevW);
        if(mainWindow->getUI()->minWidthCheckbox->isChecked())
            mainWindow->minimizeSheetWidth();
        mainWindow->updateSelectedAnim();
    }
}

bool ReverseAnimStep::isDifferent()
{
    Animation* a = mainWindow->getSheet()->getAnimation(anim);
    if(a)
        return (a->getFramePtr()->size() > 1);
    return false;
}
