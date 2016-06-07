#include "Animation.h"

Animation::Animation()
{
    sizeX = sizeY = maxWidth = 0;
    curRender = NULL;
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
}

QImage* Animation::getImage()
{
    return curRender;   //TODO test if null or whatever
}

void Animation::addFrame(QImage* img, int pos)
{
    frames.insert(pos, img);
}

QImage* Animation::getFrame(int pos)
{
    return NULL; //TODO
}

void Animation::deleteFrame(int pos)
{

}

int Animation::getFrame(int x, int y)
{
    return 0;   //TODO
}

void Animation::setTitle(QString str)
{

}

void Animation::setFont(QFont f)
{

}

void Animation::setBgTransparent(bool bTrans)
{

}

void Animation::setBgFill(QColor col)
{

}

void Animation::setSheetFill(QColor col)
{

}

void Animation::setSheetTransparent(bool bTrans)
{

}

void Animation::setSelectedCol(QColor col)
{

}

void Animation::setFrameSelectCol(QColor col)
{

}

void Animation::redraw()
{

}

void Animation::reverse()
{

}

void Animation::removeDuplicates()
{

}































