#ifndef UNDOSTEP_H
#define UNDOSTEP_H

class MainWindow;

class UndoStep
{
    UndoStep(){}
protected:
    MainWindow* mainWindow;
    bool saved;
public:
    UndoStep(MainWindow* window);

    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual bool isDifferent() = 0; //If this is a worthwhile change

    void setSaved(bool b) {saved = b;}
    bool isSaved() {return saved;}
};

#endif // UNDOSTEP_H
