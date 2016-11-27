#include "AddImagesStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

AddImagesStep::AddImagesStep(MainWindow* w, QVector<QImage> images, QString name) : UndoStep(w)
{
    toAdd = images;
    animName = name;
    insertPos = w->getSheet()->getCurSelected();
    prevW = w->getUI()->sheetWidthBox->value();
}

void AddImagesStep::undo()
{
    Sheet* sheet = mainWindow->getSheet();
    sheet->removeAnimation(insertPos);
    sheet->refresh();
    mainWindow->getUI()->sheetWidthBox->setValue(prevW);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->drawAnimation();
    mainWindow->updateSelectedAnim();
}

void AddImagesStep::redo()
{
    Sheet* sheet = mainWindow->getSheet();
    Animation* animation = new Animation(sheet->getTransparentBg(), sheet->getScene());
    QVector<QImage> imgList;
    foreach(QImage img, toAdd)
        imgList.append(img);
    animation->insertImages(imgList);
    animation->setName(animName);
    sheet->addAnimation(animation, insertPos);

    sheet->refresh();
    mainWindow->checkMinWidth();
    mainWindow->getUI()->sheetWidthBox->setValue(prevW);
    if(mainWindow->getUI()->minWidthCheckbox->isChecked())
        mainWindow->minimizeSheetWidth();
    mainWindow->drawAnimation();
    mainWindow->updateSelectedAnim();

}
