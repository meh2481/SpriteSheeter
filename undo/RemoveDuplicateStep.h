#ifndef REMOVEDUPLICATESTEP_H
#define REMOVEDUPLICATESTEP_H
#include "UndoStep.h"
#include <QVector>

class QImage;
class Animation;

class RemoveDuplicateStep : public UndoStep
{
    struct Loc {
        int pos;
        bool selected;
        QImage* img;
    };

    int idx;
    QVector<Loc> removedImages;
public:
    RemoveDuplicateStep(MainWindow* w, int animIndex);

    void undo();
    void redo();
    bool isDifferent() {return true;}   //Could calculate, but not really worth
};

#endif // REMOVEDUPLICATESTEP_H
