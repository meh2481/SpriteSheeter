#include "Sheet.h"

Sheet::Sheet(QGraphicsScene* s, QImage* bg, QObject *parent) : QObject(parent)
{
    scene = s;
    width = 1000;
    sceneRect = QRectF(0,0,0,0);
    outlineRect = backgroundRect = NULL;
    sheetBgCol = QColor(0, 128, 128, 255);  //TODO user-configure
    transparentBg = bg;
    sheetBgTransparent = false;
}

Sheet::~Sheet()
{
    foreach(Animation* animation, animations)
        delete animation;
    scene->clear(); //Handles cleaning up all the memories
}

void Sheet::addAnimation(Animation* anim, unsigned int index)
{
    if(index > animations.size())
        index = animations.size();
    animations.insert(index, anim);
    recalc();
}

void Sheet::addAnimation(Animation* anim)
{
    addAnimation(anim, animations.size());
}

void Sheet::setWidth(unsigned int w)
{
    width = w;
    recalc();
}

void Sheet::recalc()
{
    if(!animations.size())
        return; //Nothing to do TODO See if this is needed when removing anims

    unsigned int curY = 0;
    foreach(Animation* anim, animations)
    {
        anim->setOffset(0, curY);
        anim->setWidth(width);
        curY += anim->getHeight();
    }
    sceneRect.setBottom(curY);
    sceneRect.setRight(width);

    if(outlineRect == NULL)
        outlineRect = scene->addRect(sceneRect);
    else
        outlineRect->setRect(sceneRect);
    outlineRect->setZValue(-1);

    //TODO Update if existing
    if(backgroundRect == NULL)
    {
        QBrush bgTexBrush(sheetBgCol);
        if(sheetBgTransparent)
            bgTexBrush = QBrush(*transparentBg);
        backgroundRect = scene->addRect(sceneRect, QPen(), bgTexBrush);
    }
    else
        backgroundRect->setRect(sceneRect);
    backgroundRect->setZValue(-2);
}

void Sheet::setBgCol(QColor c)
{
    sheetBgCol = c;
    recalc();
}

void Sheet::setBgTransparent(bool b)
{
    sheetBgTransparent = b;
    recalc();
}

void Sheet::setXSpacing(unsigned int x)
{
    foreach(Animation* anim, animations)
        anim->setXSpacing(x);
    recalc();
}

void Sheet::setYSpacing(unsigned int y)
{
    foreach(Animation* anim, animations)
        anim->setYSpacing(y);
    recalc();
}








































