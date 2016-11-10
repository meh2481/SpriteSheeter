#ifndef ANIMATION_H
#define ANIMATION_H
#include <QObject>
#include <QVector>
#include <QImage>
#include <QGraphicsScene>
#include <QMap>
#include <QGraphicsPixmapItem>

class Animation : public QObject
{
    Q_OBJECT

    Animation(){}

    QVector<QGraphicsPixmapItem*> images;             //Actual images for this animation
    QMap<QGraphicsPixmapItem*, QImage*> imageMap;
    int offsetX, offsetY;
    int spacingX, spacingY;
    int width;

    void recalcPosition();   //Recalculate where each image is on in the sheet
public:
    explicit Animation(QObject *parent = 0);
    ~Animation();

    //Insert an image at the end of the animation and hand over control of the memory
    void insertImage(QImage* img, QGraphicsScene* scene);

    //Insert an image at the specified index and hand over control of the memory
    void insertImage(QImage* img, QGraphicsScene* scene, unsigned int index);

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




























