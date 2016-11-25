#ifndef UNDOSTEP_H
#define UNDOSTEP_H

class MainWindow;

class UndoStep
{
    UndoStep(){}
protected:
    MainWindow* mainWindow;
public:
    UndoStep(MainWindow* window);

    virtual void undo() = 0;
    virtual void redo() = 0;
};

#endif // UNDOSTEP_H
