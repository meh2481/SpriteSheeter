#include "Animation.h"
#include <QFontMetrics>

Animation::Animation()
{
    sizeX = sizeY = maxWidth = 0;
    curRender = NULL;
    bgTrans = sheetTrans = false;
}

int Animation::width()
{
    return sizeX;
}

int Animation::height()
{
    return sizeY;
}

void Animation::setMaxWidth(int width)
{
    maxWidth = width;
    redraw();
}

QImage* Animation::getImage()
{
    return curRender;
}

void Animation::addFrame(QImage* img, int pos)
{
    if(pos < 0)
        frames.append(img);
    else
        frames.insert(pos, img);
    redraw();
}

QImage* Animation::getFrame(int pos)
{
    if(pos < frames.size() || pos > 0)
        return NULL;
    return frames.at(pos);
}

void Animation::deleteFrame(int pos)
{
    if(pos < frames.size() && pos > 0)
    {
        frames.removeAt(pos);
        redraw();
    }
}

int Animation::getFrame(int x, int y)
{
    int iFrame = 0;
    foreach(QRect rc, frameRects)
    {
        if(rc.contains(x, y))
            return iFrame;
        iFrame++;
    }

    return -1;
}

void Animation::setTitle(QString str)
{
    title = str;
    redraw();
}

void Animation::setFont(QFont f)
{
    font = f;
    redraw();
}

void Animation::setBgTransparent(bool bTrans)
{
    bgTrans = bTrans;
    redraw();
}

void Animation::setBgFill(QColor col)
{
    bgFill = col;
    redraw();
}

void Animation::setSheetFill(QColor col)
{
    sheetFill = col;
    redraw();
}

void Animation::setSheetTransparent(bool bTrans)
{
    sheetTrans = bTrans;
    redraw();
}

void Animation::setSelectedCol(QColor col)
{
    selectedCol = col;
    redraw();
}

void Animation::setFrameSelectCol(QColor col)
{
    frameSelectedCol = col;
    redraw();
}

void Animation::redraw()
{
    if(curRender && (curRender->width() != sizeX || curRender->height() != sizeY))
    {
        delete curRender;
        curRender = NULL;
    }
    if(!curRender)
        curRender = new QImage(sizeX, sizeY, QImage::Format_ARGB32);

    //TODO
}

void Animation::reverse()
{
    QList<QImage*> revFrames = frames;
    frames.clear();
    foreach(QImage* img, revFrames)
        frames.prepend(img);

    recalculate();
    redraw();
}

bool Animation::removeDuplicates()
{
    bool bFoundDuplicates = false;

    for(int tester = 0; tester < frames.size(); tester++)
    {
        for(int testee = tester+1; testee < frames.size(); testee++)
        {
            if(frames.at(testee)->width() != frames.at(tester)->width() || frames.at(testee)->height() != frames.at(tester)->height()) continue;
            if(frames.at(testee)->byteCount() != frames.at(tester)->byteCount()) continue;

            if(std::strncmp((const char*)frames.at(testee)->bits(), (const char*)frames.at(tester)->bits(), frames.at(testee)->byteCount()) == 0)
            {
                bFoundDuplicates = true;
                frames.removeAt(testee);
                testee--;
            }
        }
    }

    if(bFoundDuplicates)
    {
        recalculate();
        redraw();
    }

    return bFoundDuplicates;
}

void Animation::recalculate()
{
    QFontMetrics fm(font);
    float textHeight = fm.height() + FONT_SPACING;

    frameRects.clear();

    //Figure out dimensions of final image
    sizeY = spaceY;
    if(title.length() && titleEnabled)
        sizeY += textHeight;
    sizeX = 0;
    int curSizeX = spaceX;
    int ySize = 0;
    foreach(QImage* img, frames)
    {
        //Test to see if we should start next line
        if(curSizeX + img->width() + spaceX > maxWidth)
        {
            sizeY += spaceY + ySize;
            ySize = 0;
            if(curSizeX > sizeX)
                sizeX = curSizeX;
            curSizeX = spaceX;
        }

        //Add rectangle for this image
        QRect r(sizeX, sizeY, img->width(), img->height());
        frameRects.append(r);

        if(img->height() > ySize)
            ySize = img->height();

        curSizeX += img->width() + spaceX;
    }

    if(curSizeX > sizeX)
        sizeX = curSizeX;

    sizeY += spaceY + ySize;
}

void Animation::setSpacing(int x, int y)
{
    bool changed = false;
    if(x != spaceX || y != spaceY)
        changed = true;

    spaceX = x;
    spaceY = y;

    if(changed)
    {
        recalculate();
        redraw();
    }
}

void Animation::setBgImage(QImage bg)
{
    bgImage = bg;
}

void Animation::draggingTo(int x, int y)
{
    //TODO
}

void Animation::droppedAt(int x, int y, QList<QImage*> images)
{
    //TODO
}

void Animation::balance(/*TODO*/)
{
    //TODO
}

void Animation::saveAs(QString GIFFilename)
{
    //TODO
}























