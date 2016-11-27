#include "BalanceAnimStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

BalanceAnimStep::BalanceAnimStep(MainWindow* win, int animIndex, int width, int height, BalancePos::Pos vert, BalancePos::Pos horiz) : UndoStep(win)
{
    animationIndex = animIndex;
    w = width;
    h = height;
    ve = vert;
    ho = horiz;

    origSheetWidth = win->getUI()->sheetWidthBox->value();

    Animation* anim = win->getSheet()->getAnimation(animIndex);
    QVector<Frame*> startFrames = anim->getFrames();
    foreach(Frame* f, startFrames)
    {
        frames.append(f->getImage());
        checked.append(f->isSelected());
    }
}

BalanceAnimStep::~BalanceAnimStep()
{
}

void BalanceAnimStep::undo()
{
    Animation* a = mainWindow->getSheet()->getAnimation(animationIndex);
    QVector<QImage> origFrames;
    foreach(QImage img, frames)
        origFrames.append(img);
    a->clear();
    a->insertImages(origFrames);
    for(int i = 0; i < checked.length(); i++)
    {
        Frame* f = a->getFrame(i);
        if(f && checked.at(i))
            f->selectToggle();
    }
    mainWindow->userEditingWidth = false;
    mainWindow->getUI()->sheetWidthBox->setValue(origSheetWidth);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->drawAnimation();
    mainWindow->updateSelectedAnim();
}

void BalanceAnimStep::redo()
{
    Animation* a = mainWindow->getSheet()->getAnimation(animationIndex);
    a->balance(QPoint(w, h), ve, ho);
    int minW = mainWindow->getSheet()->getSmallestPossibleWidth();
    mainWindow->userEditingWidth = false;
    if(minW > mainWindow->getUI()->sheetWidthBox->value())
        mainWindow->getUI()->sheetWidthBox->setValue(minW);
    else
        mainWindow->getUI()->sheetWidthBox->setValue(origSheetWidth);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->drawAnimation();
    mainWindow->updateSelectedAnim();
}
