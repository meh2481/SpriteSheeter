#ifndef ANIMPREVIEW_H
#define ANIMPREVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QScrollBar>

class AnimPreview : public QGraphicsView
{
    bool _pan;
    int _panStartX, _panStartY;

public:
    AnimPreview(QWidget * parent);

protected:

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
};

#endif
