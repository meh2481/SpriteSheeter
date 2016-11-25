#ifndef FONTCOLORSTEP_H
#define FONTCOLORSTEP_H
#include <QColor>
#include "UndoStep.h"

class FontColorStep : public UndoStep
{
    QColor origColor, newColor;
public:
    FontColorStep(MainWindow* window, QColor& origC, QColor& newC);

    void undo();
    void redo();
};

#endif // FONTCOLORSTEP_H
