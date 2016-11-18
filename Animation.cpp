#include "Animation.h"
#include <QDebug>
#include <QPixmap>
#include <QBrush>
#include <QPainter>

Animation::Animation(QImage* bg, QGraphicsScene* s, QObject *parent) : QObject(parent)
{
    offsetX = offsetY = 0;
    spacingX = spacingY = 0;
    width = 1000;
    frameBgCol = QColor(0, 255, 0);
    frameBgTransparent = false;
    transparentBg = bg;
    curHeight = 0;
    minWidth = 0;
    scene=s;
}

Animation::~Animation()
{
    foreach(Frame* f, frames)
        delete f;
}

void Animation::insertImage(QImage* img)
{
    insertImage(img, frames.size());
}

void Animation::insertImage(QImage* img, unsigned int index)
{
    if(index > (unsigned int)frames.size())
        index = frames.size();

    Frame* f = new Frame(scene, img, frameBgCol, transparentBg, frameBgTransparent);
    frames.insert(index, f);

    heightRecalc();
}

void Animation::insertImages(QVector<QImage*> imgs)
{
    foreach(QImage* img, imgs)
        insertImage(img);
}

void Animation::insertImages(QVector<QImage*> imgs, unsigned int index)
{
    foreach(QImage* img, imgs)
        insertImage(img, index++);
}

void Animation::pullImages(Animation* other, QList<unsigned int> indices, unsigned int insertLocation)
{
    //TODO
}

unsigned int Animation::widthOfImages()
{
    unsigned int imgWidth = 0;
    foreach(Frame* f, frames)
        imgWidth += f->getWidth();
    return imgWidth + (spacingX*(frames.size()+1));
}

unsigned int Animation::heightRecalc()
{
    int curX = spacingX;
    int curY = spacingY;
    unsigned int tallestHeight = 0;
    minWidth = widthOfImages();
    if(minWidth > (unsigned int)width)
        minWidth = 0;
    foreach(Frame* f, frames)
    {
        if(f->getWidth() + curX + spacingX > width)
        {
            curY += tallestHeight + spacingY;     //Next line
            curX = spacingX;
            tallestHeight = f->getHeight();
        }
        else if((unsigned int)f->getHeight() > tallestHeight)
            tallestHeight = f->getHeight();
        f->setPosition(curX + offsetX, curY + offsetY);
        curX += spacingX + f->getWidth();
        if(minWidth < (unsigned int)curX)
            minWidth = curX;
    }
    curHeight = curY + tallestHeight;
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
        foreach(Frame* f, frames)
            f->setFrameBgCol(c);
    }
}

void Animation::setFrameBgTransparent(bool b)
{
    if(frameBgTransparent != b)
    {
        frameBgTransparent = b;
        foreach(Frame* f, frames)
            f->setFrameBgTransparent(b);
    }
}

void Animation::setFrameBgVisible(bool b)
{
    foreach(Frame* f, frames)
        f->setFrameBgVisible(b);
}

void Animation::reverse()
{
    if(frames.size() < 2)
        return;

    QVector<Frame*> newList = frames;
    frames.clear();
    foreach(Frame* f, newList)
        frames.prepend(f);
    heightRecalc();
}

bool Animation::removeDuplicateFrames()
{
    if(frames.size() < 2)
        return false;

    bool bFoundDuplicates = false;

    for(int tester = 0; tester < frames.size(); tester++)
    {
        for(int testee = tester+1; testee < frames.size(); testee++)
        {
            Frame* testerItem = frames[tester];
            Frame* testeeItem = frames[testee];
            QImage* testerImg = testerItem->getImage();
            QImage* testeeImg = testeeItem->getImage();

            //Images of different size
            if(testeeImg->width() != testerImg->width() || testeeImg->height() != testerImg->height())
                continue;

            //Images of different byte counts
            if(testeeImg->byteCount() != testerImg->byteCount())
                continue;

            if(std::memcmp(testeeImg->bits(), testerImg->bits(), testeeImg->byteCount()) == 0)
            {
                bFoundDuplicates = true;
                frames.removeAt(testee);
                testee--;
                delete testeeItem;   //Free image
            }
        }
    }
    return bFoundDuplicates;
}

QPoint Animation::getMaxFrameSize()
{
    int w = 0, h = 0;
    foreach(Frame* f, frames)
    {
        if(f->getWidth() > w)
            w = f->getWidth();
        if(f->getHeight() > h)
            h = f->getHeight();
    }
    return QPoint(w, h);
}

void Animation::balance(QPoint sz, BalancePos::Pos vert, BalancePos::Pos horiz)
{
    unsigned int w = sz.x();
    unsigned int h = sz.y();
    foreach(Frame* f, frames)
        f->resize(w, h, vert, horiz);
    heightRecalc();
}

bool Animation::isInside(int x, int y)
{
    return (x >= offsetX &&
            x <= offsetX + width &&
            y >= offsetY &&
            y <= offsetY + curHeight);
}

unsigned int Animation::getSmallestImageWidth()
{
    return getMaxFrameSize().x();
}

void Animation::toggleSelect(QGraphicsItem* it)
{
    foreach(Frame* f, frames)
    {
        if(f->isThis(it))
        {
            f->selectToggle();
            break;
        }
    }
}

bool Animation::deleteSelected()
{
    for(int i = frames.size()-1; i >= 0; i--)
    {
        if(frames.at(i)->isSelected())
        {
            Frame* f = frames.at(i);
            frames.remove(i);
            delete f;
        }
    }
    return(!frames.size());
}

bool Animation::hasSelected()
{
    foreach(Frame* f, frames)
    {
        if(f->isSelected())
            return true;
    }
    return false;
}

bool Animation::isSelected(QGraphicsItem* it)
{
    foreach(Frame* f, frames)
    {
        if(f->isThis(it) && f->isSelected())
            return true;
    }
    return false;
}

QLine Animation::getDragPos(int x, int y)
{
    QPoint size = getMaxFrameSize();
    QLine result(-1,-1,-1,-1);
    x -= offsetX;
    y -= offsetY;
    //Before animation if near the top
    if(y-spacingY <= size.y() * ANIM_DRAG_SPACINGY)
        return QLine(offsetX, offsetY+spacingY*0.5, offsetX + width, offsetY+spacingY*0.5);
    //After animation if near the bottom
    if(y >= curHeight - (size.y() * ANIM_DRAG_SPACINGY))
        return QLine(offsetX, offsetY + curHeight+spacingY*0.5, offsetX + width, offsetY + curHeight+spacingY*0.5);
    //Position inside animation
    int curX = spacingX;
    int curY = spacingY;
    unsigned int tallestHeight = 0;
    foreach(Frame* f, frames)
    {
        if(f->getWidth() + curX + spacingX > width)
        {
            curY += tallestHeight + spacingY;     //Next line
            curX = spacingX;
            tallestHeight = f->getHeight();
        }
        else if((unsigned int)f->getHeight() > tallestHeight)
            tallestHeight = f->getHeight();
        curX += spacingX + f->getWidth();
    }

    return result;
}
