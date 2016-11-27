#ifndef REVERSEANIMSTEP_H
#define REVERSEANIMSTEP_H
#include "UndoStep.h"

class ReverseAnimStep : public UndoStep
{
    int anim;
    int prevW;  //Previous sheet width before this step
public:
    ReverseAnimStep(MainWindow* w, int curSelectedAnim);

    void undo();
    void redo();
    bool isDifferent();
};

#endif // REVERSEANIMSTEP_H
