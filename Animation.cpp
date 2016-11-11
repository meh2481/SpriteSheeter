#include "Animation.h"
#include <QDebug>
#include <QPixmap>

Animation::Animation(QObject *parent) : QObject(parent)
{
    offsetX = offsetY = 0;
    spacingX = spacingY = 0;
    width = 1000;
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

void Animation::recalcPosition()
{
    //TODO Update animation background and individual frame backgrounds
    int curX = spacingX;
    int curY = spacingY;
    foreach(QGraphicsPixmapItem* pixmapItem, images)
    {
        QImage* image = imageMap.value(pixmapItem);
        if(image->width() + curX + spacingX > width)
        {
            curY += image->height() + spacingY;     //Next line
            curX = spacingX;
        }
        pixmapItem->setPos(curX + offsetX, curY + offsetY);
        curX += spacingX + image->width();
    }
}

unsigned int Animation::getHeight()
{
    int curX = spacingX;
    int curY = spacingY;
    unsigned int tallestHeight = 0;
    foreach(QGraphicsPixmapItem* pixmapItem, images)
    {
        QImage* image = imageMap.value(pixmapItem);
        if(image->height() > tallestHeight)
            tallestHeight = image->height();
        if(image->width() + curX + spacingX > width)
        {
            curY += tallestHeight + spacingY;     //Next line
            curX = spacingX;
            tallestHeight = 0;
        }
        curX += spacingX + image->width();
    }
    return curY + spacingY + tallestHeight;
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
























