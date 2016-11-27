#ifndef BALANCEANIMSTEP_H
#define BALANCEANIMSTEP_H
#include "UndoStep.h"
#include "Animation.h"
#include "BalancePos.h"

class BalanceAnimStep : public UndoStep
{
    int animationIndex;
    QVector<QImage*> frames;
    QVector<bool> checked;
    int w, h;
    int origSheetWidth;
    BalancePos::Pos ve, ho;
public:
    BalanceAnimStep(MainWindow* win, int animIndex, int width, int height, BalancePos::Pos vert, BalancePos::Pos horiz);
    ~BalanceAnimStep();

    void undo();
    void redo();
    bool isDifferent() {return true;}   //Prolly can check this manually but whatever
};

#endif // BALANCEANIMSTEP_H
