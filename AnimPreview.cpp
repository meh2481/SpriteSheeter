#include "AnimPreview.h"

AnimPreview::AnimPreview(QWidget * parent) : QGraphicsView(parent)
{
    _pan = false;
    _panStartX = _panStartY = 0;
}

void AnimPreview::mousePressEvent(QMouseEvent *event)
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

void AnimPreview::mouseReleaseEvent(QMouseEvent *event)
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

void AnimPreview::mouseMoveEvent(QMouseEvent *event)
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
