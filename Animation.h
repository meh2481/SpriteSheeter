#ifndef ANIMATION_H
#define ANIMATION_H
#include <QObject>
#include <QVector>
#include <QImage>
#include <QGraphicsScene>
#include <QMap>
#include <QGraphicsItem>

class Animation : public QObject
{
    Q_OBJECT

    Animation(){}

    QGraphicsScene* m_scene;
    QVector<QImage*> images;             //Actual images for this animation
    QVector<QImage*> renderableImages;   //Renderable images that are drawn in the graphics view
    QMap<QGraphicsItem*, unsigned int> sceneIndices; //Mapping of scene graphics item to index into images

public:
    explicit Animation(QGraphicsScene* scene, QObject *parent = 0);
    ~Animation();

    //Insert an image at the end of the animation and hand over control of the memory
    void insertImage(QImage* img);

    //Insert an image at the specified index and hand over control of the memory
    void insertImage(QImage *img, unsigned int index);

    //Insert a list of images at the end of the animation and hand over control of the memory
    void insertImages(const QVector<QImage*>& imagesToAdd);

    //Insert a list of images at the given index and hand over control of the memory
    void insertImages(const QVector<QImage *>& imagesToAdd, unsigned int index);

    //Get the image at the given index
    QImage* getImage(unsigned int index);

    //Get the index for the given graphics item, or NULL if none exists
    unsigned int getIndex(QGraphicsItem* it);

    //Remove the given indices from this animation (returned list will be sorted by index)
    //Note the Qt syntax: indices << 33 << 12 << 68 << 6 << 12;
    QList<QImage*> pullImages(QList<unsigned int> indices);

signals:

public slots:

private:
};

#endif // ANIMATION_H




























