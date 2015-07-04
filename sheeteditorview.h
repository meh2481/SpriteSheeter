#ifndef COOLVIEW_H
#define COOLVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QScrollBar>

class sheetEditorView : public QGraphicsView
{
    bool _pan;
    int _panStartX, _panStartY;

public:
    sheetEditorView(QWidget * parent) : QGraphicsView(parent)
    {
        _pan = false;
        _panStartX = _panStartY = 0;
    }

protected:
    /*void enterEvent(QEvent *event)
    {
        QGraphicsView::enterEvent(event);
        viewport()->setCursor(Qt::ArrowCursor);
    }*/

    void mousePressEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::MidButton)
        {
            _pan = true;
            _panStartX = event->x();
            _panStartY = event->y();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        event->ignore();
    }

    void mouseReleaseEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::MidButton)
        {
            _pan = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }
        event->ignore();
    }

    void mouseMoveEvent(QMouseEvent *event)
    {
        if (_pan)
        {
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - _panStartX));
            verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - _panStartY));
            _panStartX = event->x();
            _panStartY = event->y();
            event->accept();
            return;
        }
        event->ignore();
    }
};

#endif
