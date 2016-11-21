#include "Frame.h"
#include <QPainter>

Frame::Frame(QGraphicsScene* s, QImage* i, QColor bgCol, QImage* tBg, bool frameBgTransparent)
{
    scene = s;
    img = i;
    frameBgCol = bgCol;
    transparentBg = tBg;
    x = y = 0;
    selected = false;
    bgTransparent = frameBgTransparent;
    //Add image
    item = scene->addPixmap(QPixmap::fromImage(*img));
    //Add bg rect
    bg = scene->addRect(0, 0, img->width(), img->height(), QPen(Qt::NoPen), QBrush(frameBgCol));
    bg->setZValue(-1);  //Behind images
    //Add fg rect
    fg = scene->addRect(0, 0, img->width(), img->height(), QPen(QColor(0,0,255), FRAME_SELECT_THICKNESS), QBrush(QColor(0,0,255,120)));
    fg->setZValue(1);  //In front of images
    fg->setVisible(false);
}

Frame::~Frame()
{
    scene->removeItem(item);
    scene->removeItem(fg);
    scene->removeItem(bg);
    delete img;
}

void Frame::setPosition(int xPos, int yPos)
{
    x = xPos;
    y = yPos;
    item->setPos(x, y);
    bg->setRect(x, y, img->width(), img->height());
    fg->setRect(x, y, img->width(), img->height());
}

void Frame::setFrameBgCol(QColor c)
{
    frameBgCol = c;
    if(!bgTransparent)
    {
        QBrush brush(c);
        bg->setBrush(brush);
    }
}

void Frame::resize(int w, int h, BalancePos::Pos vert, BalancePos::Pos horiz)
{
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

    QImage* final = new QImage(w, h, QImage::Format_ARGB32);
    final->fill(QColor(0,0,0,0));
    QPainter painter(final);
    painter.drawImage(xPos, yPos, *img);
    painter.end();
    item->setPixmap(QPixmap::fromImage(*final));
    delete img;
    img = final;

    //Reset frame rects
    bg->setRect(x, y, w, h);
    fg->setRect(x, y, w, h);
}

void Frame::setFrameBgVisible(bool b)
{
    bg->setVisible(b);
}

void Frame::setFrameBgTransparent(bool b)
{
    bgTransparent = b;
    if(b)
        bg->setBrush(QBrush(*transparentBg));
    else
        bg->setBrush(QBrush(frameBgCol));
}

void Frame::selectToggle()
{
    selected = !selected;
    fg->setVisible(selected);
}

void Frame::render(QPainter& painter)
{
    //Fill in bg highlight col if we should
    if(!bgTransparent)
    {
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.fillRect(x, y, img->width(), img->height(), QBrush(frameBgCol));
    }

    painter.drawImage(x, y, *img);
}
