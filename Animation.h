#ifndef ANIMATION_H
#define ANIMATION_H
#include <QObject>
#include <QVector>
#include <QImage>
#include <QGraphicsScene>
#include <QMap>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QColor>
#include <QPainter>
#include <QGraphicsSimpleTextItem>
#include <QFont>
#include "BalancePos.h"
#include "Frame.h"

#define ANIM_DRAG_SPACINGY 0.2
#define ANIM_BEFORE -1
#define ANIM_AFTER -2
#define ANIM_NONE -3

class Animation : public QObject
{
    Q_OBJECT

    Animation(){}

    QGraphicsScene* scene;
    QVector<Frame*> frames;
    int offsetX, offsetY;   //Position on the screen
    int spacingX, spacingY;
    int width;
    int curHeight;  //Last-calculated height for the animation
    QColor frameBgCol;
    bool frameBgTransparent;
    QImage* transparentBg;
    unsigned int minWidth;  //Minimum width for this animation at the current width
    QString name;
    QGraphicsSimpleTextItem* label;

    unsigned int heightRecalc();    //Recalculate where each image is on in the sheet
    unsigned int widthOfImages();
public:
    explicit Animation(QImage* bg, QGraphicsScene* s, QObject *parent = 0);
    ~Animation();

    //Insert an image at the end of the animation and hand over control of the memory
    void insertImage(QImage* img);

    //Insert an image at the specified index and hand over control of the memory
    void insertImage(QImage* img, unsigned int index);

    //Insert a list of images at the end of the animation and hand over control of the memory
    void insertImages(QVector<QImage*>& imgs);

    //Insert a list of images at the specified index and hand over control of the memory
    void insertImages(QVector<QImage*>& imgs, unsigned int index);

    //Remove the selected images from the given animation and add them to this one
    void addImages(QVector<Frame*>& imgs, unsigned int index);

    QVector<Frame*> pullSelected(int* pullLoc = NULL); //Remove selected frames from this anim

    //Set the width of the animation. Return the new height
    unsigned int setWidth(unsigned int width);

    //Set the spacing between animations and frames
    void setSpacing(unsigned int x, unsigned int y);
    void setXSpacing(unsigned int x);
    void setYSpacing(unsigned int y);

    //Set the offset to draw this animation at
    void setOffset(unsigned int x, unsigned int y);

    //Set the background color for each frame
    void setFrameBgCol(QColor c);

    //Set the background transparency for each frame
    void setFrameBgTransparent(bool b);

    //Set the visibility of the frame bg (if both sheet and frame bgs are invisible, don't draw the latter)
    void setFrameBgVisible(bool b);

    //Get the last-calculated height for the animation
    unsigned int getCurHeight() {return curHeight;}

    //Reverse the animation
    void reverse();

    //Remove duplicate frames from the current animation (return true if duplicates found/removed)
    bool removeDuplicateFrames();

    //Get the largest width/height out of all animation frames
    QPoint getMaxFrameSize();

    //Balance sheet to the given size
    //TODO don't include BalanceSheetDialog just for this
    void balance(QPoint sz, BalancePos::Pos vert, BalancePos::Pos horiz);

    //Test if a point is inside the animation
    bool isInside(int x, int y);

    unsigned int getMinWidth() {return minWidth;}   //Get the minimum width for the current width
    unsigned int getSmallestImageWidth();           //Get the smallest possible width for this animation

    bool toggleSelect(QGraphicsItem* it); //Select the given item as a frame (return selected)

    bool deleteSelected();  //Return true if now empty after deletion

    bool hasSelected(); //Return true if any frames in this animation are selected

    bool isSelected(QGraphicsItem* it);   //Return true if this item is in this sheet and selected

    QLine getDragPos(int x, int y);

    //Get location in the animation this is dropped.
    //ANIM_BEFORE if before this animation, ANIM_AFTER if after, ANIM_NONE if not on this animation
    int getDropPos(int x, int y);

    bool isEmpty() {return frames.isEmpty();}

    void deselectAll();

    QVector<Frame*>& getFrames() {return frames;}

    void render(QPainter& painter);

    QString getName() {return name;}
    void setName(QString s);

    void setFont(QFont& f);
    void setFontColor(QColor c);

signals:

public slots:

private:
};

#endif // ANIMATION_H
