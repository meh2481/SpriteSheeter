#ifndef ADDIMAGESSTEP_H
#define ADDIMAGESSTEP_H
#include "UndoStep.h"
#include <QVector>
#include <QString>

class QImage;

class AddImagesStep : public UndoStep
{
    QVector<QImage*> toAdd;
    QString animName;
    int insertPos;
    int prevW;  //Sheet width before applying this step

public:
    AddImagesStep(MainWindow* w, QVector<QImage*> images, QString name);

    void undo();
    void redo();
    bool isDifferent() {return (toAdd.size() > 0);}
};

#endif // ADDIMAGESSTEP_H
