#ifndef FONTCOLORSTEP_H
#define FONTCOLORSTEP_H
#include <QColor>
#include "UndoStep.h"

class FontColorStep : public UndoStep
{
    QColor origColor, newColor;
public:
    FontColorStep(MainWindow* window, QColor const & origC, QColor const & newC);

    void undo();
    void redo();
    bool isDifferent() {return origColor != newColor;}
};

#endif // FONTCOLORSTEP_H
