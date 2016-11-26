#include "AnimNameStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

AnimNameStep::AnimNameStep(MainWindow* w, QString origName, QString newName, int anim) : UndoStep(w)
{
    prev = origName;
    cur = newName;
    curAnim = anim;
}

void AnimNameStep::undo()
{
    mainWindow->getUI()->animationNameEditor->setText(prev);
    Animation* anim = mainWindow->getSheet()->getAnimation(curAnim);
    anim->setName(prev);
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}

void AnimNameStep::redo()
{
    mainWindow->getUI()->animationNameEditor->setText(cur);
    Animation* anim = mainWindow->getSheet()->getAnimation(curAnim);
    anim->setName(cur);
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->updateSelectedAnim();
}
