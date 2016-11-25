#include "ReverseAnimStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

ReverseAnimStep::ReverseAnimStep(MainWindow* w, int curSelectedAnim) : UndoStep(w)
{
    anim = curSelectedAnim;
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
        mainWindow->updateSelectedAnim();
    }
}
