#ifndef SHEET_H
#define SHEET_H

#include <QObject>
#include <QVector>
#include <QGraphicsRectItem>
#include <QRectF>
#include <QColor>
#include <QImage>
#include "Animation.h"

class Sheet : public QObject
{
    Q_OBJECT

    QVector<Animation*> animations;
    QGraphicsScene* scene;

    unsigned int width;

    Sheet(){}   //Private default constructor
    void recalc();

    //QGraphicsRectItem* outlineRect;
    QGraphicsRectItem* backgroundRect;

    QRectF sceneRect;
    QColor sheetBgCol;
    QImage* transparentBg;
    bool sheetBgTransparent;
    unsigned int xSpacing, ySpacing;

public:
    explicit Sheet(QGraphicsScene* s, QImage* bg, QObject *parent = 0);
    ~Sheet();

    void addAnimation(Animation* anim);
    void addAnimation(Animation* anim, unsigned int index);

    void setWidth(unsigned int w);
    void setBgCol(QColor c);
    void setBgTransparent(bool b);

    void setXSpacing(unsigned int x);
    void setYSpacing(unsigned int y);

signals:

public slots:
};

#endif // SHEET_H
