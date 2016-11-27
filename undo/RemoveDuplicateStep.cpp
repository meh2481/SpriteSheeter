#include "RemoveDuplicateStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

RemoveDuplicateStep::RemoveDuplicateStep(MainWindow* w, int animIndex) : UndoStep(w)
{
    idx = animIndex;
    prevW = w->getUI()->sheetWidthBox->value();
}

void RemoveDuplicateStep::undo()
{
    Animation* anim = mainWindow->getSheet()->getAnimation(idx);
    //Insert back in reverse order
    for(int i = removedImages.size() - 1; i >= 0; i--)
    {
        Loc l = removedImages.at(i);
        anim->insertImage(l.img, l.pos);
        if(l.selected)
            anim->toggleSelect(l.pos);
    }
    removedImages.clear();

    mainWindow->getSheet()->refresh();
    mainWindow->mAnimFrame = 0;
    mainWindow->drawAnimation();
    mainWindow->getUI()->sheetWidthBox->setValue(prevW);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->updateSelectedAnim();
}

void RemoveDuplicateStep::redo()
{
    Animation* anim = mainWindow->getSheet()->getAnimation(idx);
    bool bFoundDuplicates = false;
    QVector<Frame*>* frames = anim->getFramePtr();
    for(int tester = 0; tester < frames->size(); tester++)
    {
        for(int testee = tester+1; testee < frames->size(); testee++)
        {
            Frame* testerItem = frames->at(tester);
            Frame* testeeItem = frames->at(testee);
            QImage testerImg = testerItem->getImage();
            QImage testeeImg = testeeItem->getImage();

            //Images of different size
//            if(testeeImg.width() != testerImg.width() || testeeImg.height() != testerImg->height())
//                continue;

//            //Images of different byte counts
//            if(testeeImg->byteCount() != testerImg->byteCount())
//                continue;

//            if(std::memcmp(testeeImg->bits(), testerImg->bits(), testeeImg->byteCount()) == 0)
            if(testerImg == testeeImg)
            {
                bFoundDuplicates = true;
                Loc l;
                l.img = testeeImg;
                l.pos = testee;
                l.selected = testeeItem->isSelected();
                removedImages.append(l);
                frames->removeAt(testee);
                testee--;
                delete testeeItem;
            }
        }
    }

    if(bFoundDuplicates)
    {
        mainWindow->getSheet()->refresh();
        if(mainWindow->getUI()->minWidthCheckbox->isChecked())
            mainWindow->minimizeSheetWidth();
        mainWindow->mAnimFrame = 0;
        mainWindow->drawAnimation();
        mainWindow->updateSelectedAnim();
    }

}
