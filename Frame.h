#ifndef FRAME_H
#define FRAME_H
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QImage>
#include <QColor>
#include "BalancePos.h"

class Frame
{
    //Empty private default constructor
    Frame(){}

    QGraphicsScene* scene;
    QGraphicsPixmapItem* item;
    QGraphicsRectItem* bg;
    QGraphicsRectItem* fg;
    QImage* img;
    bool selected;
    int x, y;
    bool bgTransparent;
    QColor frameBgCol;
    QImage* transparentBg;

public:
    Frame(QGraphicsScene* s, QImage* i, QColor bgCol, QImage* tBg, bool frameBgTransparent);
    ~Frame();

    void setPosition(int xPos, int yPos);

    int getWidth() {return img->width();}
    int getHeight() {return img->height();}

    void setFrameBgCol(QColor c);

    void resize(int w, int h, BalancePos::Pos vert, BalancePos::Pos horiz);
    QImage* getImage() {return img;}

    void setFrameBgVisible(bool b);

    void setFrameBgTransparent(bool b);

    void selectToggle();

    bool isThis(QGraphicsItem* it) {return it==item;}

    bool isSelected() {return selected;}
};

#endif // FRAME_H
