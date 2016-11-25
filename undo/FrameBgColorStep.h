#ifndef FRAMEBGCOLORSTEP_H
#define FRAMEBGCOLORSTEP_H
#include <QColor>
#include "UndoStep.h"

class FrameBgColorStep : public UndoStep
{
    QColor origColor, newColor;
public:
    FrameBgColorStep(MainWindow* w, QColor origC, QColor newC);

    void undo();
    void redo();
};

#endif // FRAMEBGCOLORSTEP_H
