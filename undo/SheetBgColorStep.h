#ifndef SHEETBGCOLORSTEP_H
#define SHEETBGCOLORSTEP_H
#include "UndoStep.h"
#include <QColor>

class SheetBgColorStep : public UndoStep
{
    QColor origColor, newColor;
public:
    SheetBgColorStep(MainWindow* w, QColor orig, QColor newC);

    void undo();
    void redo();
    bool isDifferent() {return origColor != newColor;}
};

#endif // SHEETBGCOLORSTEP_H
