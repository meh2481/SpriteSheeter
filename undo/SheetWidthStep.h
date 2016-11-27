#ifndef SHEETWIDTHSTEP_H
#define SHEETWIDTHSTEP_H
#include "UndoStep.h"

class SheetWidthStep : public UndoStep
{
    int prev, next;
    bool different;
public:
    SheetWidthStep(MainWindow* w, int prevW, int curW);

    void undo();
    void redo();
    bool isDifferent() {return different;}
};

#endif // SHEETWIDTHSTEP_H
