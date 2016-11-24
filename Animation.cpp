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
    scene = s;
    label = scene->addSimpleText(name);
    label->setZValue(5);    //Above errything
}

Animation::~Animation()
{
    foreach(Frame* f, frames)
        delete f;
    scene->removeItem(label);
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

void Animation::insertImages(QVector<QImage*>& imgs)
{
    foreach(QImage* img, imgs)
        insertImage(img);
}

void Animation::insertImages(QVector<QImage*>& imgs, unsigned int index)
{
    foreach(QImage* img, imgs)
        insertImage(img, index++);
}

void Animation::addImages(QVector<Frame*>& imgs, unsigned int index)
{
    foreach(Frame* f, imgs)
        frames.insert(index++, f);
    heightRecalc();
}

QVector<Frame*> Animation::pullSelected(int* pullLoc)
{
    QVector<Frame*> imgList;

    for(int i = 0; i < frames.size(); i++)
    {
        Frame* f = frames.at(i);
        if(f->isSelected())
        {
            if(pullLoc != NULL && *pullLoc > i)
                (*pullLoc)--;
            imgList.append(f);
            frames.remove(i);
            i--;
        }
    }

    return imgList;
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
    if(!name.isEmpty())
        curY += label->boundingRect().height() + spacingY;
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
    if(!name.isEmpty())
    {
        //curHeight += label->boundingRect().height() + spacingY;
        label->setPos(spacingX, offsetY + spacingY);
    }
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
    foreach(Frame* f, frames)
        f->setFrameBgCol(c);
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

bool Animation::toggleSelect(QGraphicsItem* it)
{
    foreach(Frame* f, frames)
    {
        if(f->isThis(it))
        {
            f->selectToggle();
            return f->isSelected();
        }
    }
    return false;
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
    if(!name.isEmpty())
        curY += label->boundingRect().height() + spacingY;
    unsigned int tallestHeight = 0;
    for(int i = 0; i < frames.size(); i++)
    {
        Frame* f = frames.at(i);
        if(f->getWidth() + curX + spacingX > width)
        {
            curY += tallestHeight + spacingY;     //Next line
            curX = spacingX;
            tallestHeight = f->getHeight();
        }
        else if((unsigned int)f->getHeight() > tallestHeight)
            tallestHeight = f->getHeight();

        //Test to see if we're near this frame
        //Current pos y
        if(y <= curY + f->getHeight() + spacingY/2.0)
        {
            int runningTotalW = curX + f->getWidth() + spacingX;
            //Find tallest image on this line (unbalanced animations)
            for(int j = i+1; j < frames.size(); j++)
            {
                if(frames.at(j)->getWidth() + spacingX > width)
                    break;
                runningTotalW += frames.at(j)->getWidth() + spacingX;
                if((unsigned int)frames.at(j)->getHeight() > tallestHeight)
                    tallestHeight = frames.at(j)->getHeight();
            }
            //Calculate positions for before & after this frame
            int startY = offsetY + curY - spacingY / 2.0;
            int endY = startY + tallestHeight + spacingY;
            int startX = offsetX + curX - spacingX / 2.0;
            int endX = startX + f->getWidth() + spacingX;
            QLine before(startX, startY, startX, endY);
            QLine after(endX, startY, endX, endY);
            //Before current frame
            if(x < curX + f->getWidth() / 2.0)
                return before;
            //After current frame
            if(x <= curX + f->getWidth() + spacingX / 2.0)
                return after;
            if(i < frames.size()-1)
            {
                Frame* next = frames.at(i+1);
                //At end of current line
                if(curX + f->getWidth() + next->getWidth() + spacingX*2 > width)
                    return after;
            }
            else    //End of animation
                return after;
        }

        curX += spacingX + f->getWidth();
    }

    return QLine(-1,-1,-1,-1);
}

int Animation::getDropPos(int x, int y)
{
    QPoint size = getMaxFrameSize();

    x -= offsetX;
    y -= offsetY;
    //Before animation if near the top
    if(y-spacingY <= size.y() * ANIM_DRAG_SPACINGY)
        return ANIM_BEFORE;
    //After animation if near the bottom
    if(y >= curHeight - (size.y() * ANIM_DRAG_SPACINGY))
        return ANIM_AFTER;
    //Position inside animation
    int curX = spacingX;
    int curY = spacingY;
    if(!name.isEmpty())
        curY += label->boundingRect().height() + spacingY;
    unsigned int tallestHeight = 0;
    for(int i = 0; i < frames.size(); i++)
    {
        Frame* f = frames.at(i);
        if(f->getWidth() + curX + spacingX > width)
        {
            curY += tallestHeight + spacingY;     //Next line
            curX = spacingX;
            tallestHeight = f->getHeight();
        }
        else if((unsigned int)f->getHeight() > tallestHeight)
            tallestHeight = f->getHeight();

        //Test to see if we're near this frame
        //Current pos y
        if(y <= curY + f->getHeight() + spacingY/2.0)
        {
            //Before current frame
            if(x < curX + f->getWidth() / 2.0)
                return i;
            //After current frame
            if(x <= curX + f->getWidth() + spacingX / 2.0)
                return i+1;
            if(i < frames.size()-1)
            {
                Frame* next = frames.at(i+1);
                //At end of current line
                if(curX + f->getWidth() + next->getWidth() + spacingX*2 > width)
                    return i+1;
            }
            else    //End of animation
                return i+1;
        }

        curX += spacingX + f->getWidth();
    }

    return ANIM_NONE;
}

void Animation::deselectAll()
{
    foreach(Frame* f, frames)
    {
        if(f->isSelected())
            f->selectToggle();
    }
}

void Animation::render(QPainter& painter)
{
    //TODO Render anim title

    foreach(Frame* f, frames)
        f->render(painter);
}

void Animation::setName(QString s)
{
    name = s;
    label->setText(name);
    heightRecalc();
}

void Animation::setFont(QFont& f)
{
    label->setFont(f);
}

void Animation::setFontColor(QColor c)
{
    label->setBrush(QBrush(c));
}
