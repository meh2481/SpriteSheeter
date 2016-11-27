#ifndef DELETESTEP_H
#define DELETESTEP_H
#include "UndoStep.h"
#include <QVector>
#include <QMap>
#include <QSet>
#include <QImage>
class Animation;

class DeleteStep : public UndoStep
{
    struct DeleteLoc {
        QImage img;
        int anim;
        int frame;
    };

    QMap<int, QSet<int> > framesToDelete;
    QVector<DeleteLoc> deletedFrames;
    QVector<int> deletedAnimations;
    int prevW;  //previous width of sheet

    void clear();
    void deleteSelected();
    QVector<DeleteLoc> deleteSelectedFrames(Animation* anim, int animIdx);

public:
    DeleteStep(MainWindow* w);
    ~DeleteStep();

    void undo();
    void redo();
    bool isDifferent();
};

#endif // DELETESTEP_H
