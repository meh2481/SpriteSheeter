#ifndef SHEETEDITORVIEW_H
#define SHEETEDITORVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QScrollBar>

class SheetEditorView : public QGraphicsView
{
    Q_OBJECT

    bool _pan;
    int _panStartX, _panStartY;

public:
    explicit SheetEditorView(QWidget *parent = 0);
    ~SheetEditorView();

    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);

signals:
    void mouseMoved(int x, int y);
    void mousePressed(int x, int y);
    void mouseReleased(int x, int y);
    void droppedFiles(QStringList sl);
    void droppedFolders(QStringList sl);

protected:

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
};

#endif
