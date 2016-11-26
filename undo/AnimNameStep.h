#ifndef ANIMNAMESTEP_H
#define ANIMNAMESTEP_H
#include <QString>
#include "UndoStep.h"

class AnimNameStep : public UndoStep
{
    QString prev, cur;
    int curAnim;
public:
    AnimNameStep(MainWindow* w, QString origName, QString newName, int anim);

    void undo();
    void redo();
};

#endif // ANIMNAMESTEP_H
