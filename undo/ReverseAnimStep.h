#ifndef REVERSEANIMSTEP_H
#define REVERSEANIMSTEP_H
#include "UndoStep.h"

class ReverseAnimStep : public UndoStep
{
    int anim;
public:
    ReverseAnimStep(MainWindow* w, int curSelectedAnim);

    void undo();
    void redo();
    bool isDifferent();
};

#endif // REVERSEANIMSTEP_H
