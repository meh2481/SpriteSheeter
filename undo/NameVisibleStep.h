#ifndef NAMEVISIBLESTEP_H
#define NAMEVISIBLESTEP_H
#include "UndoStep.h"

class NameVisibleStep : public UndoStep
{
    bool newVisible;
public:
    NameVisibleStep(MainWindow* w, bool visible);

    void undo();
    void redo();
    bool isDifferent() {return true;}
};

#endif // NAMEVISIBLESTEP_H
