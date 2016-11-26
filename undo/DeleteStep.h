#ifndef DELETESTEP_H
#define DELETESTEP_H
#include "UndoStep.h"
#include <QVector>
class QImage;
class Animation;

class DeleteStep : public UndoStep
{
    struct DeleteLoc {
        QImage* img;
        int anim;
        int frame;
    };

    QVector<DeleteLoc> deletedFrames;
    QVector<int> deletedAnimations;

    void clear();
    void deleteSelected();
    QVector<DeleteLoc> deleteSelectedFrames(Animation* anim);

public:
    DeleteStep(MainWindow* w);
    ~DeleteStep();

    void undo();
    void redo();
};

#endif // DELETESTEP_H
