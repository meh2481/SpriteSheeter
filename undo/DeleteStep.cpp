#include "DeleteStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

DeleteStep::DeleteStep(MainWindow* w) : UndoStep(w)
{

}

DeleteStep::~DeleteStep()
{
    clear();
}

void DeleteStep::undo()
{
    Sheet* sheet = mainWindow->getSheet();
    Ui::MainWindow* ui = mainWindow->getUI();

    //Add back animations
    for(int i = deletedAnimations.size() - 1; i >= 0; i--)
    {
        int deleted = deletedAnimations.at(i);
        Animation* anim = new Animation(sheet->getTransparentBg(), sheet->getScene());
        sheet->addAnimation(anim, deleted);
    }

    //Add back frames
    for(int i = deletedFrames.size() - 1; i >= 0; i--)
    {
        DeleteLoc dl = deletedFrames.at(i);
        Animation* anim = sheet->getAnimation(dl.anim);
        anim->insertImage(new QImage(dl.img->copy()), dl.frame);
        anim->getFrame(dl.frame)->selectToggle();   //By definition will always be selected
    }

    if(ui->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    sheet->refresh();
    sheet->updateSceneBounds();
    mainWindow->drawAnimation();
    mainWindow->updateSelectedAnim();
}

void DeleteStep::redo()
{
    clear();
    Ui::MainWindow* ui = mainWindow->getUI();
    Sheet* sheet = mainWindow->getSheet();

    deleteSelected();
    if(ui->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    sheet->refresh();
    sheet->updateSceneBounds();
    mainWindow->drawAnimation();
    mainWindow->updateSelectedAnim();
}

void DeleteStep::clear()
{
    foreach(DeleteLoc dl, deletedFrames)
        delete dl.img;
    deletedFrames.clear();
    deletedAnimations.clear();
}

void DeleteStep::deleteSelected()
{
    Sheet* sheet = mainWindow->getSheet();
    QVector<Animation*>* animations = sheet->getAnimationPtr();
    for(int i = animations->size() - 1; i >= 0; i--)
    {
        QVector<DeleteLoc> deleted = deleteSelectedFrames(animations->at(i));
        foreach(DeleteLoc dl, deleted)
        {
            dl.anim = i;
            deletedFrames.prepend(dl);
        }

        if(animations->at(i)->isEmpty())
        {
            delete animations->at(i);
            animations->remove(i);
            deletedAnimations.prepend(i);   //prepend cause reverse order
        }
    }
}

QVector<DeleteStep::DeleteLoc> DeleteStep::deleteSelectedFrames(Animation* anim)
{
    QVector<DeleteLoc> deleted;
    QVector<Frame*>* frames = anim->getFramePtr();
    for(int i = frames->size() - 1; i >= 0; i--)
    {
        if(frames->at(i)->isSelected())
        {
            Frame* f = frames->at(i);
            frames->remove(i);
            DeleteLoc dl;
            dl.anim = 0;
            dl.frame = i;
            dl.img = new QImage(f->getImage()->copy());
            deleted.prepend(dl);    //prepend cause reverse order
            delete f;
        }
    }
    return deleted;
}
