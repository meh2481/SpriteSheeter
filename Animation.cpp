#include "Animation.h"
#include <QDebug>
#include <QPixmap>
#include <QBrush>
#include <QPainter>

Animation::Animation(QImage* bg, QObject *parent) : QObject(parent)
{
    offsetX = offsetY = 0;
    spacingX = spacingY = 0;
    width = 1000;
    frameBgCol = QColor(0, 255, 0);
    frameBgTransparent = false;
    transparentBg = bg;
    curHeight = 0;
}

Animation::~Animation()
{
    //Graphics scene cleans up after itself already
    for(QMap<QGraphicsPixmapItem*, QImage*>::iterator img = imageMap.begin(); img != imageMap.end(); img++)
        delete img.value(); //Image memory, however, does not
}

void Animation::insertImage(QImage* img, QGraphicsScene* scene)
{
    insertImage(img, scene, images.size());
}

void Animation::insertImage(QImage* img, QGraphicsScene* scene, unsigned int index)
{
    if(index > (unsigned int)images.size())
        index = images.size();
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(*img));
    images.insert(index, item);
    imageMap.insert(item, img);
    scene->addItem(item);
    //Add background for this also
    QBrush brush(frameBgCol);
    if(frameBgTransparent)
        brush = QBrush(*transparentBg);
    QGraphicsRectItem* bgRect = scene->addRect(0, 0, img->width(), img->height(), QPen(Qt::NoPen), brush);
    bgRect->setZValue(-1);  //Behind images
    frameBackgrounds.insert(index, bgRect);
    heightRecalc();
}

void Animation::insertImages(QVector<QImage*> imgs, QGraphicsScene* scene)
{
    foreach(QImage* img, imgs)
        insertImage(img, scene);
}

void Animation::insertImages(QVector<QImage*> imgs, QGraphicsScene* scene, unsigned int index)
{
    foreach(QImage* img, imgs)
        insertImage(img, scene, index++);
}

//TODO I don't think this is correct anymore
void Animation::pullImages(Animation* other, QList<unsigned int> indices, unsigned int insertLocation)
{
    if(insertLocation > (unsigned int)images.length())
        insertLocation = images.length();

    qSort(indices); //Sort
    std::reverse(indices.begin(), indices.end());   //Reverse

    foreach(unsigned int i, indices)
    {
        QGraphicsPixmapItem* img = other->images.at(i);
        images.insert(insertLocation, img);
        other->images.remove(i);
    }
    heightRecalc();
    other->heightRecalc();
}

unsigned int Animation::heightRecalc()
{
    int curX = spacingX;
    int curY = spacingY;
    unsigned int tallestHeight = 0;
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
        else if((unsigned int)image->height() > tallestHeight)
            tallestHeight = image->height();
        pixmapItem->setPos(curX + offsetX, curY + offsetY);
        frameBackgrounds.at(i)->setRect(curX + offsetX, curY + offsetY, image->width(), image->height());
        curX += spacingX + image->width();
    }
    curHeight = curY + spacingY + tallestHeight;
    return curHeight;
}

unsigned int Animation::setWidth(unsigned int w)
{
    width = w;
    return heightRecalc();
}

void Animation::setOffset(unsigned int x, unsigned int y)
{
    offsetX = x;
    offsetY = y;
    heightRecalc();
}

void Animation::setSpacing(unsigned int x, unsigned int y)
{
    spacingX = x;
    spacingY = y;
    heightRecalc();
}

void Animation::setXSpacing(unsigned int x)
{
    if(spacingX != x)
    {
        spacingX = x;
        heightRecalc();
    }
}

void Animation::setYSpacing(unsigned int y)
{
    if(spacingY != y)
    {
        spacingY = y;
        heightRecalc();
    }
}

void Animation::setFrameBgCol(QColor c)
{
    frameBgCol = c;
    if(!frameBgTransparent)
    {
        QBrush brush(c);
        foreach(QGraphicsRectItem* bg, frameBackgrounds)
            bg->setBrush(brush);
    }
}

void Animation::setFrameBgTransparent(bool b)
{
    if(frameBgTransparent != b)
    {
        frameBgTransparent = b;
        QBrush brush(frameBgCol);
        if(frameBgTransparent)
            brush = QBrush(*transparentBg);
        foreach(QGraphicsRectItem* it, frameBackgrounds)
            it->setBrush(brush);
    }
}

void Animation::setFrameBgVisible(bool b)
{
    foreach(QGraphicsRectItem* it, frameBackgrounds)
        it->setVisible(b);
}

void Animation::reverse()
{
    QVector<QGraphicsPixmapItem*> newList = images;
    images.clear();
    foreach(QGraphicsPixmapItem* img, newList)
        images.prepend(img);
    heightRecalc();
}

bool Animation::removeDuplicateFrames()
{
    if(images.size() < 2)
        return false;

    bool bFoundDuplicates = false;

    for(int tester = 0; tester < images.size(); tester++)
    {
        for(int testee = tester+1; testee < images.size(); testee++)
        {
            QGraphicsPixmapItem* testerItem = images[tester];
            QGraphicsPixmapItem* testeeItem = images[testee];
            QImage* testerImg = imageMap.value(testerItem);
            QImage* testeeImg = imageMap.value(testeeItem);

            //Images of different size
            if(testeeImg->width() != testerImg->width() || testeeImg->height() != testerImg->height())
                continue;

            //Images of different byte counts
            if(testeeImg->byteCount() != testerImg->byteCount())
                continue;

            if(std::memcmp(testeeImg->bits(), testerImg->bits(), testeeImg->byteCount()) == 0)
            {
                bFoundDuplicates = true;
                //Remove testee
                images.removeAt(testee);
                QGraphicsRectItem* rectItem = frameBackgrounds.at(testee);
                frameBackgrounds.removeAt(testee);
                testee--;
                //Remove from scene
                QGraphicsScene* scene = testeeItem->scene();
                scene->removeItem(testeeItem);
                scene->removeItem(rectItem);
                imageMap.remove(testeeItem);
                delete testeeImg;   //Free image
            }
        }
    }
    return bFoundDuplicates;
}

QPoint Animation::getMaxFrameSize()
{
    int w = 0, h = 0;
    for(QMap<QGraphicsPixmapItem*, QImage*>::iterator img = imageMap.begin(); img != imageMap.end(); img++)
    {
        if(img.value()->width() > w)
            w = img.value()->width();
        if(img.value()->height() > h)
            h = img.value()->height();
    }
    return QPoint(w, h);
}

void Animation::balance(QPoint sz, BalancePos::Pos vert, BalancePos::Pos horiz)
{
    unsigned int w = sz.x();
    unsigned int h = sz.y();
    foreach(QGraphicsPixmapItem* item, images)
    {
//        qDebug() << "balance() loop begin" << endl;
        QImage* img = imageMap.value(item);

        //Use vert/horiz
        int xPos, yPos;
        if(vert == BalancePos::Up)
            yPos = 0;
        else if(vert == BalancePos::Mid)
            yPos = (h/2)-(img->height()/2);
        else
            yPos = h - img->height();

        if(horiz == BalancePos::Left)
            xPos = 0;
        else if(horiz == BalancePos::Mid)
            xPos = (w/2)-(img->width()/2);
        else
            xPos = w - img->width();

//        qDebug() << "balance() create final image" << endl;
        QImage* final = new QImage(w, h, QImage::Format_ARGB32);
        final->fill(QColor(0,0,0,0));
        QPainter painter(final);
//        qDebug() << "balance() draw new img size" << endl;
        painter.drawImage(xPos, yPos, *img);
        painter.end();
        item->setPixmap(QPixmap::fromImage(*final));
        imageMap.insert(item, final);
        delete img;
    }
    heightRecalc();
}

bool Animation::isInside(int x, int y)
{
    return (x >= offsetX &&
            x <= offsetX + width &&
            y >= offsetY &&
            y <= offsetY + curHeight);
}
