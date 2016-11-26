#include "DragDropStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

DragDropStep::DragDropStep(MainWindow* w, int x, int y) : UndoStep(w)
{
    
    animOverIdx = w->getSheet()->getOver(x, y);
    dropLocation = w->getSheet()->getAnimation(animOverIdx)->getDropPos(x, y);
    animAddedTo = animCreated = -1;
}

void DragDropStep::undo()
{

}

void DragDropStep::redo()
{
    Sheet* sheet = mainWindow->getSheet();
    QVector<Animation*>* animations = sheet->getAnimationPtr();
    //Pull frames from animations
    Animation* over = animations->at(animOverIdx);
    int location = dropLocation;
    int curAnim = 0;
    foreach(Animation* anim, *animations)
    {
        QVector<FrameLoc> frames = pullSelected(anim, (anim == over)?(&location):(NULL));
        foreach(FrameLoc f, frames)
        {
            f.anim = curAnim;
            movedFrames.append(f);
        }
        curAnim++;
    }

    //Figure out what to do with them
    if(location >= 0)                   //Add to this anim
    {
        QVector<QImage*> pulledImages = getPulledImages();
        over->insertImages(pulledImages, location);
        selectFrames(over, location, pulledImages.size());
        animAddedTo = animOverIdx;
        animCreated = -1;
    }
    else if(location == ANIM_BEFORE)    //Add before this anim
    {
        Animation* anim = new Animation(sheet->getTransparentBg(), sheet->getScene());
        QVector<QImage*> pulledImages = getPulledImages();
        anim->insertImages(pulledImages, 0);
        selectFrames(anim, 0, pulledImages.size());
        sheet->addAnimation(anim, animOverIdx);
        animAddedTo = animCreated = animOverIdx;
    }
    else if(location == ANIM_AFTER)     //Add after this anim
    {
        Animation* anim = new Animation(sheet->getTransparentBg(), sheet->getScene());
        QVector<QImage*> pulledImages = getPulledImages();
        anim->insertImages(pulledImages, 0);
        selectFrames(anim, 0, pulledImages.size());
        sheet->addAnimation(anim, animOverIdx + 1);
        animAddedTo = animCreated = animOverIdx+1;
    }

    //Delete animations that are empty as a result of this
    sheet->deleteEmpty();   //TODO hold onto these

    //Recalculate sheet positions
    sheet->refresh();
    mainWindow->updateSelectedAnim();
}

QVector<DragDropStep::FrameLoc> DragDropStep::pullSelected(Animation* anim, int* pullLoc)
{
    QVector<FrameLoc> imgList;

    QVector<Frame*>* frames = anim->getFramePtr();
    for(int i = 0; i < frames->size(); i++)
    {
        Frame* f = frames->at(i);
        if(f->isSelected())
        {
            if(pullLoc != NULL && *pullLoc > i)
                (*pullLoc)--;
            FrameLoc fl;
            fl.frame = i;
            fl.img = new QImage(f->getImage()->copy());
            fl.selected = f->isSelected();
            imgList.append(fl);
            frames->remove(i);
            i--;
            delete f;
        }
    }

    return imgList;
}

QVector<QImage*> DragDropStep::getPulledImages()
{
    QVector<QImage*> imgs;
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
