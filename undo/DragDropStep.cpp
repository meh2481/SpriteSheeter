#include "DragDropStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

DragDropStep::DragDropStep(MainWindow* w, int x, int y) : UndoStep(w)
{
    animOverIdx = w->getSheet()->getOver(x, y);
    origDropLocation = w->getSheet()->getAnimation(animOverIdx)->getDropPos(x, y);
    animAddedTo = animCreated = -1;

    readSelectedFrames();
    prevW = w->getUI()->sheetWidthBox->value();
}

DragDropStep::~DragDropStep()
{
    clear();
}

void DragDropStep::undo()
{
    Sheet* sheet = mainWindow->getSheet();
    //See if we created a new anim
    if(animCreated >= 0)
        sheet->removeAnimation(animCreated);
    else
    {
        Animation* anim = sheet->getAnimation(animAddedTo);
        //Remove added frames
        for(int i = 0; i < movedFrames.size(); i++)
            anim->removeFrame(newDropLocation);
    }

    //Loop through reverse, adding back in deleted animations
    for(int i = deletedAnimations.size() - 1; i >= 0; i--)
    {
        int deleted = deletedAnimations.at(i);
        Animation* anim = new Animation(sheet->getTransparentBg(), sheet->getScene());
        anim->setName(deletedAnimNames.at(i));
        anim->setNameVisible(sheet->areNamesVisible());
        sheet->addAnimation(anim, deleted);
    }

    //Loop through reverse, adding back in deleted frames
    for(int j = movedFrames.size() - 1; j >= 0; j--)
    {
        FrameLoc fl = movedFrames.at(j);
        Animation* a = sheet->getAnimation(fl.anim);
        a->insertImage(fl.img, fl.frame);
        if(fl.selected)
            a->getFrame(fl.frame)->selectToggle();
    }
    //Recalculate sheet positions
    sheet->refresh();
    mainWindow->userEditingWidth = false;
    mainWindow->getUI()->sheetWidthBox->setValue(prevW);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->updateSelectedAnim();
}

void DragDropStep::redo()
{
    clear();    //Wipe clean if we've done this before
    newDropLocation = origDropLocation;
    Sheet* sheet = mainWindow->getSheet();
    QVector<Animation*>* animations = sheet->getAnimationPtr();
    //Pull frames from animations
    Animation* over = animations->at(animOverIdx);
    int curAnim = 0;
    foreach(Animation* anim, *animations)
    {
        QVector<FrameLoc> frames = pullSelected(anim, (anim == over)?(&newDropLocation):(NULL), curAnim);
        foreach(FrameLoc f, frames)
            movedFrames.append(f);
        curAnim++;
    }

    //Figure out what to do with them
    if(newDropLocation >= 0)                   //Add to this anim
    {
        QVector<QImage> pulledImages = getPulledImages();
        over->insertImages(pulledImages, newDropLocation);
        selectFrames(over, newDropLocation, pulledImages.size());
        animAddedTo = animOverIdx;
        animCreated = -1;
    }
    else if(newDropLocation == ANIM_BEFORE)    //Add before this anim
    {
        Animation* anim = new Animation(sheet->getTransparentBg(), sheet->getScene());
        QVector<QImage> pulledImages = getPulledImages();
        anim->insertImages(pulledImages, 0);
        selectFrames(anim, 0, pulledImages.size());
        sheet->addAnimation(anim, animOverIdx);
        animAddedTo = animCreated = animOverIdx;
    }
    else if(newDropLocation == ANIM_AFTER)     //Add after this anim
    {
        Animation* anim = new Animation(sheet->getTransparentBg(), sheet->getScene());
        QVector<QImage> pulledImages = getPulledImages();
        anim->insertImages(pulledImages, 0);
        selectFrames(anim, 0, pulledImages.size());
        sheet->addAnimation(anim, animOverIdx + 1);
        animAddedTo = animCreated = animOverIdx+1;
    }

    //Delete animations that are empty as a result of this
    deleteEmpty();

    //Recalculate sheet positions
    sheet->refresh();
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->updateSelectedAnim();
}

QVector<DragDropStep::FrameLoc> DragDropStep::pullSelected(Animation* anim, int* pullLoc, int curAnim)
{
    QVector<FrameLoc> imgList;

    QVector<Frame*>* frames = anim->getFramePtr();
    QSet<int> selected = selectedFrames.value(curAnim);
    int iter = 0;
    int sz = frames->size();
    for(int i = 0; i < sz; i++)
    {
        Frame* f = frames->at(iter);
        if(selected.contains(i))
        {
            if(pullLoc != NULL && *pullLoc > iter)
                (*pullLoc)--;
            FrameLoc fl;
            fl.anim = curAnim;
            fl.frame = iter;
            fl.img = f->getImage();
            fl.selected = f->isSelected();
            imgList.append(fl);
            frames->remove(iter);
            iter--;
            delete f;
        }
        iter++;
    }

    return imgList;
}

QVector<QImage> DragDropStep::getPulledImages()
{
    QVector<QImage> imgs;
    foreach(FrameLoc f, movedFrames)
        imgs.append(f.img);
    return imgs;
}

void DragDropStep::selectFrames(Animation* anim, int loc, int count)
{
    for(int i = 0; i < count; i++)
    {
        int animLoc = i + loc;
        if(movedFrames.at(i).selected)
            anim->getFrame(animLoc)->selectToggle();
    }
}

void DragDropStep::deleteEmpty()
{
    deletedAnimations.clear();
    deletedAnimNames.clear();

    QVector<Animation*>* animations = mainWindow->getSheet()->getAnimationPtr();
    for(int i = 0; i < animations->size(); i++)
    {
        if(animations->at(i)->isEmpty())
        {
            deletedAnimNames << animations->at(i)->getName();
            delete animations->at(i);
            animations->remove(i);
            deletedAnimations << i;
            i--;
        }
    }
}

void DragDropStep::clear()
{
    movedFrames.clear();
    deletedAnimations.clear();
    deletedAnimNames.clear();
}

void DragDropStep::readSelectedFrames()
{
    Sheet* sheet = mainWindow->getSheet();
    QVector<Animation*>* animations = sheet->getAnimationPtr();
    for(int i = 0; i < animations->length(); i++)
    {
        QSet<int> sel;
        QVector<Frame*>* frames = animations->at(i)->getFramePtr();
        for(int j = 0; j < frames->length(); j++)
        {
            if(frames->at(j)->isSelected())
                sel.insert(j);
        }
        selectedFrames.insert(i, sel);
    }
}
