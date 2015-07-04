#ifndef COOLVIEW_H
#define COOLVIEW_H

#include <QGraphicsView>

class sheetEditorView : public QGraphicsView
{
public:
    sheetEditorView(QWidget * parent) : QGraphicsView(parent)
    {
    }

protected:
    void enterEvent(QEvent *event)
    {
        QGraphicsView::enterEvent(event);
        viewport()->setCursor(Qt::ArrowCursor);
    }

    void mousePressEvent(QMouseEvent *event)
    {
        QGraphicsView::mousePressEvent(event);
        viewport()->setCursor(Qt::ArrowCursor);
    }

    void mouseReleaseEvent(QMouseEvent *event)
    {
        QGraphicsView::mouseReleaseEvent(event);
        viewport()->setCursor(Qt::ArrowCursor);
    }
};

#endif
