#ifndef XSPACINGSTEP_H
#define XSPACINGSTEP_H
#include "UndoStep.h"

class XSpacingStep : public UndoStep
{
    int prevX, curX, prevW;

public:
    XSpacingStep(MainWindow* w, int previousX, int currentX, int previousSheetW);

    void undo();
    void redo();
    bool isDifferent() {return prevX != curX;}
};

#endif // XSPACINGSTEP_H
