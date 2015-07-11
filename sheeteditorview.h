#ifndef SHEETEDITORVIEW_H
#define SHEETEDITORVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QScrollBar>

class sheetEditorView : public QGraphicsView
{
    Q_OBJECT

    bool _pan;
    int _panStartX, _panStartY;

public:
    explicit sheetEditorView(QWidget *parent = 0);
    ~sheetEditorView();

    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);

signals:
    void mouseMoved(int x, int y);
    void mousePressed(int x, int y);
    void mouseReleased(int x, int y);
    void droppedFiles(QStringList sl);

protected:

    void mousePressEvent(QMouseEvent *event)
    {
        if(event->button() == Qt::MidButton)
        {
            _pan = true;
            _panStartX = event->x();
            _panStartY = event->y();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        if(event->button() == Qt::LeftButton)
        {
            QPointF pt = mapToScene(event->pos());
            mousePressed(pt.x(), pt.y());
        }
        event->ignore();
    }

    void mouseReleaseEvent(QMouseEvent *event)
    {
        if(event->button() == Qt::MidButton)
        {
            _pan = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }
        if(event->button() == Qt::LeftButton)
        {
            QPointF pt = mapToScene(event->pos());
            mouseReleased(pt.x(), pt.y());
        }
        event->ignore();
    }

    void mouseMoveEvent(QMouseEvent *event)
    {
        if(_pan)
        {
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - _panStartX));
            verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - _panStartY));
            _panStartX = event->x();
            _panStartY = event->y();
            event->accept();
            return;
        }
        QPointF pt = mapToScene(event->pos());
        mouseMoved(pt.x(), pt.y());
        event->ignore();
    }
};

#endif
