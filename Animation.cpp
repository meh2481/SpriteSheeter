#include "Animation.h"
#include <QDebug>
#include <QPixmap>
#include <QBrush>
#include <QPainter>
#include "FreeImage.h"

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
    drawLabel = true;
    frameBgVisible = true;
}

Animation::~Animation()
{
    clear();
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
    f->setFrameBgCol(frameBgCol);
    f->setFrameBgTransparent(frameBgTransparent);
    f->setFrameBgVisible(frameBgVisible);
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
    if(!name.isEmpty() && drawLabel)
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
    if(!name.isEmpty() && drawLabel)
        label->setPos(offsetX + spacingX, offsetY + spacingY);
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
    frameBgVisible = b;
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

bool Animation::toggleSelect(int pos)
{
    Frame* f = frames.at(pos);
    f->selectToggle();
    return f->isSelected();
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
    if(!name.isEmpty() && drawLabel)
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
    if(!name.isEmpty() && drawLabel)
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
    //Render anim title
    if(!name.isEmpty() && drawLabel)
    {
        QPen orig = painter.pen();
        painter.setPen(QPen(label->brush().color()));
        painter.setFont(label->font());
        painter.drawText(QRectF(offsetX + spacingX, offsetY + spacingY, width, label->boundingRect().height() + spacingY), Qt::AlignLeft|Qt::AlignTop, name);
        painter.setPen(orig);
    }

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

FIBITMAP* imageFromPixels(uint8_t* imgData, uint32_t width, uint32_t height)
{
    FIBITMAP* curImg = FreeImage_Allocate(width, height, 32);
    FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(curImg);
    if(image_type == FIT_BITMAP)
    {
        int curPos = 0;
        unsigned pitch = FreeImage_GetPitch(curImg);
        BYTE* bits = (BYTE*)FreeImage_GetBits(curImg);
        bits += pitch * height - pitch;
        for(int y = height-1; y >= 0; y--)
        {
            BYTE* pixel = (BYTE*)bits;
            for(uint32_t x = 0; x < width; x++)
            {
                pixel[FI_RGBA_BLUE] = imgData[curPos++];
                pixel[FI_RGBA_GREEN] = imgData[curPos++];
                pixel[FI_RGBA_RED] = imgData[curPos++];
                pixel[FI_RGBA_ALPHA] = imgData[curPos++];
                //HACK Quantize low-alpha to magenta by hand...
                if(pixel[FI_RGBA_ALPHA] <= 128)
                {
                    pixel[FI_RGBA_RED] = 255;
                    pixel[FI_RGBA_GREEN] = 0;
                    pixel[FI_RGBA_BLUE] = 255;
                    pixel[FI_RGBA_ALPHA] = 255;
                }
                pixel += 4;
            }
            bits -= pitch;
        }
    }
    return curImg;
}

void Animation::saveGIF(QString saveFilename, int animFPS)
{
    //Open GIF image for writing
    FIMULTIBITMAP* bmp = FreeImage_OpenMultiBitmap(FIF_GIF, saveFilename.toStdString().c_str(), true, false);

    DWORD dwFrameTime = (DWORD)((1000.0f / (float)animFPS) + 0.5f); //Framerate of gif image

    foreach(Frame* f, frames)
    {
        //Create image and 256-color image
        QImage imgTemp(*(f->getImage()));
        //Gotta get Qt image in proper format first
        imgTemp = imgTemp.convertToFormat(QImage::Format_ARGB32);
        QByteArray bytes((char*)imgTemp.bits(), imgTemp.byteCount());
        //HACK Make 32-bit image with magenta instead of transparency first...
        FIBITMAP* page = imageFromPixels((uint8_t*)bytes.data(), imgTemp.width(), imgTemp.height());
        //Turn this into an 8-bit image next
        FIBITMAP* page8bit = FreeImage_ColorQuantize(page, FIQ_WUQUANT);

        //Set transparency table from magenta. *Hopefully this was preserved during quantization!*
        RGBQUAD *Palette = FreeImage_GetPalette(page8bit);
        BYTE Transparency[256];
        for (unsigned i = 0; i < 256; i++)
        {
            Transparency[i] = 0xFF;
            if(Palette[i].rgbGreen == 0x00 &&
                    Palette[i].rgbBlue == 0xFF &&
                    Palette[i].rgbRed == 0xFF)
            {
                Transparency[i] = 0x00;
            }
        }
        FreeImage_SetTransparencyTable(page8bit, Transparency, 256);

        //Append metadata - frame speed based on current playback speed
        FreeImage_SetMetadata(FIMD_ANIMATION, page8bit, NULL, NULL);
        FITAG *tag = FreeImage_CreateTag();
        if(tag)
        {
            FreeImage_SetTagKey(tag, "FrameTime");
            FreeImage_SetTagType(tag, FIDT_LONG);
            FreeImage_SetTagCount(tag, 1);
            FreeImage_SetTagLength(tag, 4);
            FreeImage_SetTagValue(tag, &dwFrameTime);
            FreeImage_SetMetadata(FIMD_ANIMATION, page8bit, FreeImage_GetTagKey(tag), tag);
            FreeImage_DeleteTag(tag);
        }
        FreeImage_AppendPage(bmp, page8bit);
        FreeImage_Unload(page);
        FreeImage_Unload(page8bit);
    }

    //Save final GIF
    FreeImage_CloseMultiBitmap(bmp, GIF_DEFAULT);
}

void Animation::setNameVisible(bool b)
{
    drawLabel = b;
    label->setVisible(b);
}

void Animation::clear()
{
    foreach(Frame* f, frames)
        delete f;
    frames.clear();
}

Frame* Animation::getFrame(unsigned int index)
{
    if(index < (unsigned int)frames.size())
        return frames.at(index);
    if(frames.size())
        return frames.at(frames.size()-1);
    return NULL;
}

void Animation::removeFrame(int index)
{
    if(index < 0 || index > frames.size() - 1)
        return;

    delete frames.at(index);
    frames.remove(index);
}
