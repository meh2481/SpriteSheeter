#ifndef SHEETWIDTHSTEP_H
#define SHEETWIDTHSTEP_H
#include "UndoStep.h"

class SheetWidthStep : public UndoStep
{
    int prev, next;
public:
    SheetWidthStep(MainWindow* w, int prevW, int curW);

    void undo();
    void redo();
    bool isDifferent() {return prev != next;}
};

#endif // SHEETWIDTHSTEP_H
