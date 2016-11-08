#include "Animation.h"

Animation::Animation(QObject *parent) : QObject(parent)
{
    offsetX = offsetY = 0;
}

Animation::~Animation()
{
    foreach(QGraphicsItem* img, images)
        delete img;
    //TODO Remove from graphics scene also
}

void Animation::insertImage(QGraphicsItem* img)
{
    insertImage(img, images.size());
}

void Animation::insertImage(QImage *QGraphicsItem, unsigned int index)
{
    if(index > images.size())
        index = images.size();
    images.insert(index, img);
    recalcPosition();
}

void Animation::insertImages(const QVector<QGraphicsItem*>& imagesToAdd)
{
    insertImages(imagesToAdd, imagesToAdd.size());
}

void Animation::insertImages(const QVector<QGraphicsItem *>& imagesToAdd, unsigned int index)
{
    if(index > imagesToAdd.size())
        index = imagesToAdd.size();
    foreach(QGraphicsItem* img, imagesToAdd)
        images.insert(index++, img);    //Increment index here to insert in order
    recalcPosition();
}

QGraphicsItem* Animation::getImage(unsigned int index)
{
    if(index < images.size())
        return images.at(index);
    return NULL;
}

unsigned int Animation::getIndex(QGraphicsItem* img)
{
    for(int i = 0; i < images.length(); i++)
    {
        if(images.at(i) == img)
            return i;
    }
    return NULL;
}

void Animation::pullImages(Animation* other, QList<unsigned int> indices, unsigned int insertLocation)
{
    if(insertLocation > images.length())
        insertLocation = images.length();

    qSort(indices); //Sort
    std::reverse(indices.begin(), indices.end());   //Reverse

    foreach(unsigned int i, indices)
    {
        QGraphicsItem* img = other->images.at(i);
        images.insert(img, insertLocation);
        other->images.remove(i);
    }
    recalcPosition();
    other->recalcPosition();
}

void Animation::recalcPosition()
{
    unsigned int curX = spacingX;
    unsigned int curY = spacingY;
    foreach(QGraphicsItem* img, images)
    {
        unsigned int imgW = img->shape().boundingRect().width();
        unsigned int imgH = img->shape().boundingRect().height();
        if(imgW + curX + spacingX > width)
            curY += imgH + spacingY;     //Next line
        img->setPos(curX + offsetX, curY + offsetY);
        curX += spacingX;
    }
}

unsigned int Animation::getHeight()
{
    unsigned int curX = spacingX;
    unsigned int curY = spacingY;
    foreach(QGraphicsItem* img, images)
    {
        unsigned int imgW = img->shape().boundingRect().width();
        unsigned int imgH = img->shape().boundingRect().height();
        if(imgW + curX + spacingX > width)
            curY += imgH + spacingY;     //Next line
        curX += spacingX;
    }
    return curY + spacingY;
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



























