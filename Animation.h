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

    QVector<QGraphicsItem*> images;             //Actual images for this animation
    unsigned int offsetX, offsetY;
    unsigned int spacingX, spacingY;
    unsigned int width;

    recalcPosition();   //Recalculate where each image is on in the sheet
public:
    explicit Animation(QObject *parent = 0);
    ~Animation();

    //Insert an image at the end of the animation and hand over control of the memory
    void insertImage(QGraphicsItem* img);

    //Insert an image at the specified index and hand over control of the memory
    void insertImage(QGraphicsItem *img, unsigned int index);

    //Insert a list of images at the end of the animation and hand over control of the memory
    void insertImages(const QVector<QGraphicsItem*>& imagesToAdd);

    //Insert a list of images at the given index and hand over control of the memory
    void insertImages(const QVector<QGraphicsItem *>& imagesToAdd, unsigned int index);

    //Get the image at the given index (Note: O(n))
    QGraphicsItem* getImage(unsigned int index);

    //Get the index for the given graphics item, or NULL if none exists
    unsigned int getIndex(QGraphicsItem* img);

    //Remove the given images from the given animation and add them to this one
    //Note the Qt syntax: otherIndices << 33 << 12 << 68 << 6 << 12;
    void pullImages(Animation* other, QList<unsigned int> otherIndices, unsigned int insertLocation);

    //Get the height for the current width
    unsigned int getHeight();

    //Set the width of the animation
    void setWidth(unsigned int width);

    //Set the spacing between animations and frames
    void setSpacing(unsigned int x, unsigned int y);

    //Set the offset to draw this animation at
    void setOffset(unsigned int x, unsigned int y);

signals:

public slots:

private:
};

#endif // ANIMATION_H




























