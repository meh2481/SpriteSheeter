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

    //Graphics item for background color
    QGraphicsRectItem* backgroundRect;

    QRectF sceneRect;
    QColor sheetBgCol;
    QColor frameBgCol;
    bool sheetBgTransparent;
    bool frameBgTransparent;
    QImage* transparentBg;
    unsigned int xSpacing, ySpacing;

    void updateAnimBg();
public:
    explicit Sheet(QGraphicsScene* s, QImage* bg, QObject *parent = 0);
    ~Sheet();

    void addAnimation(Animation* anim);
    void addAnimation(Animation* anim, unsigned int index);

    void setWidth(unsigned int w);
    void setBgCol(QColor c);
    void setFrameBgCol(QColor c);
    void setBgTransparent(bool b);
    void setFrameBgTransparent(bool b);

    void setXSpacing(unsigned int x);
    void setYSpacing(unsigned int y);

signals:

public slots:
};

#endif // SHEET_H
