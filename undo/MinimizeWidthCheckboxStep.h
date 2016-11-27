#ifndef MINIMIZEWIDTHCHECKBOXSTEP_H
#define MINIMIZEWIDTHCHECKBOXSTEP_H
#include "UndoStep.h"

class MinimizeWidthCheckboxStep : public UndoStep
{
    bool prev, next;
    int lastW;

public:
    MinimizeWidthCheckboxStep(MainWindow* w, bool prevChecked, bool nextChecked, int lastSheetW);

    void undo();
    void redo();
    bool isDifferent() {return prev != next;}
};

#endif // MINIMIZEWIDTHCHECKBOXSTEP_H
