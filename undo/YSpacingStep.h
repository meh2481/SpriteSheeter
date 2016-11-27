#ifndef YSPACINGSTEP_H
#define YSPACINGSTEP_H
#include "UndoStep.h"

class YSpacingStep : public UndoStep
{
    int orig, current;
public:
    YSpacingStep(MainWindow* w, int originalSpacing, int currentSpacing);

    void undo();
    void redo();
    bool isDifferent() {return orig != current;}
};

#endif // YSPACINGSTEP_H
