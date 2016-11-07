#include "Animation.h"

Animation::Animation(QGraphicsScene *scene, QObject *parent) : QObject(parent)
{
    m_scene = scene;
}

Animation::~Animation()
{
    foreach(QImage* img, images)
        delete img;
}

void Animation::insertImage(QImage* img)
{
    insertImage(img, images.size());
}

void Animation::insertImage(QImage *img, unsigned int index)
{
    if(index > images.size())
        index = images.size();
    images.insert(index, img);
}

void Animation::insertImages(const QVector<QImage*>& imagesToAdd)
{
    insertImages(imagesToAdd, imagesToAdd.size());
}

void Animation::insertImages(const QVector<QImage *>& imagesToAdd, unsigned int index)
{
    if(index > imagesToAdd.size())
        index = imagesToAdd.size();
    foreach(QImage* img, imagesToAdd)
        images.insert(index++, img);    //Increment index here to insert in order. Sick
}

QImage* Animation::getImage(unsigned int index)
{
    if(index < images.size())
        return images.at(index);
    return NULL;
}

unsigned int Animation::getIndex(QGraphicsItem* it)
{
    return sceneIndices.value(it, NULL);
}

QList<QImage*> Animation::pullImages(QList<unsigned int> indices)
{
    QList<QImage*> ret;

    qSort(indices); //Sort
    std::reverse(indices.begin(), indices.end());   //Reverse

    foreach(unsigned int i, indices)
    {
        if(i < images.size())
            ret.prepend(images.at(i));
    }

    foreach(unsigned int i, indices)
    {
        if(i < images.size())
            images.remove(i);
    }

    return ret;
}


































