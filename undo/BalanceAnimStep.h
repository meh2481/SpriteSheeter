#ifndef BALANCEANIMSTEP_H
#define BALANCEANIMSTEP_H
#include "UndoStep.h"
#include "Animation.h"
#include "BalancePos.h"

class BalanceAnimStep : public UndoStep
{
    Animation* a;
    QVector<QImage*> frames;
    QVector<bool> checked;
    int w, h;
    BalancePos::Pos ve, ho;
public:
    BalanceAnimStep(MainWindow* win, Animation* anim, int width, int height, BalancePos::Pos vert, BalancePos::Pos horiz);
    ~BalanceAnimStep();

    void undo();
    void redo();
};

#endif // BALANCEANIMSTEP_H
