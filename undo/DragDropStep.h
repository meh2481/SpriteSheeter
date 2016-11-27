#ifndef DRAGDROPSTEP_H
#define DRAGDROPSTEP_H
#include "UndoStep.h"
#include <QVector>
#include <QMap>
#include <QSet>
class QImage;
class Animation;

class DragDropStep : public UndoStep
{
    struct FrameLoc {
        QImage* img;
        int anim;
        int frame;
        bool selected;
    };

    int animOverIdx;
    int origDropLocation;
    int newDropLocation;
    int animAddedTo;
    int animCreated;
    QVector<FrameLoc> movedFrames;
    QVector<int> deletedAnimations;
    QMap<int, QSet<int> > selectedFrames;

    QVector<FrameLoc> pullSelected(Animation* anim, int* pullLoc, int curAnim);
    QVector<QImage*> getPulledImages();
    void selectFrames(Animation* anim, int loc, int count);

    void clear();
    void readSelectedFrames();

public:
    DragDropStep(MainWindow* w, int x, int y);
    ~DragDropStep();

    void undo();
    void redo();
    bool isDifferent() {return true;}   //Prolly can check this manually
};

#endif // DRAGDROPSTEP_H
