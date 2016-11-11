#include "Animation.h"
#include <QDebug>
#include <QPixmap>
#include <QBrush>

Animation::Animation(QObject *parent) : QObject(parent)
{
    offsetX = offsetY = 0;
    spacingX = spacingY = 0;
    width = 1000;
    frameBgCol = QColor(0, 255, 0);
}

Animation::~Animation()
{
    //Graphics scene cleans up after itself already
    for(QMap<QGraphicsPixmapItem*, QImage*>::iterator img = imageMap.begin(); img != imageMap.end(); img++)
        delete img.value(); //Image memory, however, is not
}

void Animation::insertImage(QImage* img, QGraphicsScene* scene)
{
    insertImage(img, scene, images.size());
}

void Animation::insertImage(QImage* img, QGraphicsScene* scene, unsigned int index)
{
    if(index > images.size())
        index = images.size();
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(*img));
    images.insert(index, item);
    imageMap.insert(item, img);
    scene->addItem(item);
    //Add background for this also
    QGraphicsRectItem* bgRect = scene->addRect(0, 0, img->width(), img->height(), QPen(Qt::NoPen), QBrush(frameBgCol));
    bgRect->setZValue(-1);  //Behind images
    frameBackgrounds.insert(index, bgRect);
    recalcPosition();
}

void Animation::pullImages(Animation* other, QList<unsigned int> indices, unsigned int insertLocation)
{
    if(insertLocation > images.length())
        insertLocation = images.length();

    qSort(indices); //Sort
    std::reverse(indices.begin(), indices.end());   //Reverse

    foreach(unsigned int i, indices)
    {
        QGraphicsPixmapItem* img = other->images.at(i);
        images.insert(insertLocation, img);
        other->images.remove(i);
    }
    recalcPosition();
    other->recalcPosition();
}

unsigned int Animation::heightRecalc(bool setPos)
{
    //TODO Update animation background and individual frame backgrounds
    int curX = spacingX;
    int curY = spacingY;
    unsigned int tallestHeight = 0;
    //foreach(QGraphicsPixmapItem* pixmapItem, images)
    for(int i = 0; i < images.size(); i++)
    {
        QGraphicsPixmapItem* pixmapItem = images.at(i);
        QImage* image = imageMap.value(pixmapItem);
        if(image->width() + curX + spacingX > width)
        {
            curY += tallestHeight + spacingY;     //Next line
            curX = spacingX;
            tallestHeight = image->height();
        }
        else if(image->height() > tallestHeight)
            tallestHeight = image->height();
        if(setPos)
        {
            pixmapItem->setPos(curX + offsetX, curY + offsetY);
            frameBackgrounds.at(i)->setPos(curX + offsetX, curY + offsetY);
        }
        curX += spacingX + image->width();
    }
    return curY + spacingY + tallestHeight;
}

void Animation::recalcPosition()
{
    heightRecalc(true);
}

unsigned int Animation::getHeight()
{
    return heightRecalc(false);
}

void Animation::setWidth(unsigned int w)
{
    width = w;
    recalcPosition();
}

void Animation::setOffset(unsigned int x, unsigned int y)
{
     offsetX = x;
     offsetY = y;
     recalcPosition();
}

void Animation::setSpacing(unsigned int x, unsigned int y)
{
    spacingX = x;
    spacingY = y;
    recalcPosition();
}

void Animation::setXSpacing(unsigned int x)
{
    if(spacingX != x)
    {
        spacingX = x;
        recalcPosition();
    }
}

void Animation::setYSpacing(unsigned int y)
{
    if(spacingY != y)
    {
        spacingY = y;
        recalcPosition();
    }
}
























