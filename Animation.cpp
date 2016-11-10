#include "Animation.h"
#include <QDebug>

Animation::Animation(QObject *parent) : QObject(parent)
{
    offsetX = offsetY = 0;
    spacingX = spacingY = 0;
    width = 500;
}

Animation::~Animation()
{
    //Graphics scene cleans up after itself already
}

void Animation::insertImage(QGraphicsPixmapItem* img)
{
    insertImage(img, images.size());
}

void Animation::insertImage(QGraphicsPixmapItem* img, unsigned int index)
{
    if(index > images.size())
        index = images.size();
    images.insert(index, img);
    recalcPosition();
}

void Animation::insertImages(const QVector<QGraphicsPixmapItem*>& imagesToAdd)
{
    insertImages(imagesToAdd, imagesToAdd.size());
}

void Animation::insertImages(const QVector<QGraphicsPixmapItem*>& imagesToAdd, unsigned int index)
{
    if(index > imagesToAdd.size())
        index = imagesToAdd.size();
    foreach(QGraphicsPixmapItem* img, imagesToAdd)
        images.insert(index++, img);    //Increment index here to insert in order
    recalcPosition();
}

QGraphicsPixmapItem* Animation::getImage(unsigned int index)
{
    if(index < images.size())
        return images.at(index);
    return NULL;
}

unsigned int Animation::getIndex(QGraphicsPixmapItem* img)
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
        QGraphicsPixmapItem* img = other->images.at(i);
        images.insert(insertLocation, img);
        other->images.remove(i);
    }
    recalcPosition();
    other->recalcPosition();
}

void Animation::recalcPosition()
{
    int curX = spacingX;
    int curY = spacingY;
    foreach(QGraphicsPixmapItem* img, images)
    {
        //TODO This is off and (I think) takes alpha into account. Take actual image size instead.
        int imgW = img->shape().boundingRect().width();
        int imgH = img->shape().boundingRect().height();
        if(imgW + curX + spacingX > width)
        {
            curY += imgH + spacingY;     //Next line
            curX = spacingX;
        }
        img->setPos(curX + offsetX, curY + offsetY);
        curX += spacingX + imgW;
    }
}

unsigned int Animation::getHeight()
{
    int curX = spacingX;
    int curY = spacingY;
    foreach(QGraphicsPixmapItem* img, images)
    {
        int imgW = img->shape().boundingRect().width();
        int imgH = img->shape().boundingRect().height();
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



























