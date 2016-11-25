#ifndef SHEETBGTRANSPARENTSTEP_H
#define SHEETBGTRANSPARENTSTEP_H
#include "UndoStep.h"

class SheetBgTransparentStep : public UndoStep
{
    bool previousTransparent;
    bool newTransparent;
public:
    SheetBgTransparentStep(MainWindow* w, bool prev, bool next);

    void undo();
    void redo();
};

#endif // SHEETBGTRANSPARENTSTEP_H
