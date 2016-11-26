#include "BalanceAnimStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

BalanceAnimStep::BalanceAnimStep(MainWindow* win, Animation* anim, int width, int height, BalancePos::Pos vert, BalancePos::Pos horiz) : UndoStep(win)
{
    a = anim;
    w = width;
    h = height;
    ve = vert;
    ho = horiz;

    QVector<Frame*> startFrames = anim->getFrames();
    foreach(Frame* f, startFrames)
    {
        frames.append(new QImage(f->getImage()->copy()));
        checked.append(f->isSelected());
    }
}

BalanceAnimStep::~BalanceAnimStep()
{
    foreach(QImage* i, frames)
        delete i;
}

void BalanceAnimStep::undo()
{
    QVector<QImage*> origFrames;
    foreach(QImage* img, frames)
        origFrames.append(new QImage(img->copy()));
    a->clear();
    a->insertImages(origFrames);
    for(int i = 0; i < checked.length(); i++)
    {
        Frame* f = a->getFrame(i);
        if(f && checked.at(i))
            f->selectToggle();
    }
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->drawAnimation();
    mainWindow->updateSelectedAnim();
}

void BalanceAnimStep::redo()
{
    a->balance(QPoint(w, h), ve, ho);
    mainWindow->getSheet()->refresh();
    mainWindow->getSheet()->updateSceneBounds();
    mainWindow->drawAnimation();
    mainWindow->updateSelectedAnim();
}
