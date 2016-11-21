#include "Sheet.h"
#include <QBuffer>

#define SCENE_BOUNDS    300

Sheet::Sheet(QGraphicsScene* s, SheetEditorView* sheetView, QImage* bg, unsigned int dragW, QObject *parent) : QObject(parent)
{
    scene = s;
    width = curHeight = 0;
    sceneRect = QRectF(0,0,0,0);
    backgroundRect = dragRect = NULL;
    sheetBgCol = QColor(0, 128, 128, 255);
    frameBgCol = QColor(0, 255, 0);
    transparentBg = bg;
    sheetBgTransparent = frameBgTransparent = false;
    xSpacing = ySpacing = 0;
    dragRectWidth = dragW;
    sheetPreview = sheetView;
}

Sheet::~Sheet()
{
    clear();
    //scene->clear(); //Handles cleaning up all the memories
}

void Sheet::addAnimation(Animation* anim, unsigned int index)
{
    if(index > (unsigned int)animations.size())
        index = animations.size();
    animations.insert(index, anim);
    anim->setSpacing(xSpacing, ySpacing);
    anim->setFrameBgCol(frameBgCol);
    anim->setFrameBgTransparent(frameBgTransparent);
    anim->setFrameBgVisible(!frameBgTransparent || !sheetBgTransparent);
    recalc();
    updateSceneBounds();
}

void Sheet::addAnimation(Animation* anim)
{
    addAnimation(anim, animations.size());
}

void Sheet::setWidth(unsigned int w)
{
    width = w;
    recalc();
}

void Sheet::recalc()
{
    unsigned int useW = width;
    if(!animations.size())
        useW = curHeight = 0;

    unsigned int curY = 0;
    foreach(Animation* anim, animations)
    {
        anim->setOffset(0, curY);
        curY += anim->setWidth(useW);
    }
    if(curY)
        curY += ySpacing;
    sceneRect.setBottom(curY);
    sceneRect.setRight(useW);

    if(backgroundRect == NULL)
    {
        QBrush bgTexBrush(sheetBgCol);
        if(sheetBgTransparent)
            bgTexBrush = QBrush(*transparentBg);
        backgroundRect = scene->addRect(sceneRect, QPen(Qt::NoPen), bgTexBrush);
        backgroundRect->setZValue(-3);  //Always behind images and outline
    }
    else
        backgroundRect->setRect(sceneRect);

    QRectF dragRectPos(sceneRect);
    dragRectPos.setLeft(sceneRect.x() + sceneRect.width());
    dragRectPos.setRight(dragRectPos.x() + dragRectWidth);
    if(dragRect == NULL)
    {
        QBrush bgTexBrush(QColor(0,255,255));   //TODO User-configure
        dragRect = scene->addRect(dragRectPos, QPen(Qt::NoPen), bgTexBrush);
        dragRect->setZValue(-3);  //Always behind images and outline
    }
    else
        dragRect->setRect(dragRectPos);

    curHeight = curY;
}

void Sheet::setBgCol(QColor c)
{
    sheetBgCol = c;
    if(backgroundRect && !sheetBgTransparent)
        backgroundRect->setBrush(QBrush(sheetBgCol));
    recalc();
}

void Sheet::setFrameBgCol(QColor c)
{
    frameBgCol = c;
    foreach(Animation* animation, animations)
        animation->setFrameBgCol(c);
}

void Sheet::setBgTransparent(bool b)
{
    if(sheetBgTransparent != b)
    {
        if(backgroundRect)
        {
            if(b)
                backgroundRect->setBrush(QBrush(*transparentBg));
            else
                backgroundRect->setBrush(QBrush(sheetBgCol));
        }
        sheetBgTransparent = b;
        recalc();

        //Set animations in sheet bg as needed
        updateAnimBg();
    }
}

void Sheet::setXSpacing(unsigned int x)
{
    foreach(Animation* anim, animations)
        anim->setXSpacing(x);
    xSpacing = x;
    recalc();
    updateSceneBounds();
}

void Sheet::setYSpacing(unsigned int y)
{
    foreach(Animation* anim, animations)
        anim->setYSpacing(y);
    ySpacing = y;
    recalc();
    updateSceneBounds();
}

void Sheet::setFrameBgTransparent(bool b)
{
    if(frameBgTransparent != b)
    {
        frameBgTransparent = b;
        foreach(Animation* anim, animations)
            anim->setFrameBgTransparent(frameBgTransparent);
        updateAnimBg();
    }
}

void Sheet::updateAnimBg()
{
    bool frameBgHidden = frameBgTransparent && sheetBgTransparent;
    foreach(Animation* anim, animations)
        anim->setFrameBgVisible(!frameBgHidden);
}

void Sheet::updateSceneBounds()
{
    if(!width)
    {
        scene->setSceneRect(0,0,0,0);
        return;
    }
    //Set the new rect of the scene
    //Scale based on minimum scene bounds, and current viewport aspect ratio
    int scene_bounds = SCENE_BOUNDS;
    if(scene_bounds < width/2.0)
        scene_bounds = width/2.0;
    if(scene_bounds < curHeight/2.0)
        scene_bounds = curHeight/2.0;
    float hFac = (float)sheetPreview->width()/(float)sheetPreview->height();
    scene->setSceneRect(-scene_bounds*hFac, -scene_bounds, width+scene_bounds*hFac*2, curHeight+scene_bounds*2);
}

Animation* Sheet::getAnimation(unsigned int index)
{
    if(index < (unsigned int)animations.size())
        return animations.at(index);
    return NULL;
}

unsigned int Sheet::getMinWidth()
{
    unsigned int minWidth = 0;
    foreach(Animation* anim, animations)
    {
        if(minWidth < anim->getMinWidth())
            minWidth = anim->getMinWidth();
    }
    return minWidth;
}

unsigned int Sheet::getSmallestPossibleWidth()
{
    unsigned int minWidth = 0;
    foreach(Animation* anim, animations)
    {
        unsigned int animSmallest = anim->getSmallestImageWidth();
        if(minWidth < animSmallest)
            minWidth = animSmallest;
    }
    return minWidth + xSpacing*2;
}

bool Sheet::clicked(int x, int y, QGraphicsItem* it)
{
    Q_UNUSED(x)

    int curY = 0;
    Animation* over = NULL;
    foreach(Animation* anim, animations)
    {
        curY += anim->getCurHeight();
        if(curY > y)
        {
            over = anim;
            break;
        }
    }

    if(over)
        return over->toggleSelect(it);
    return false;
}

void Sheet::deleteSelected()
{
    for(int i = animations.size()-1; i >= 0; i--)
    {
        if(animations.at(i)->deleteSelected())
            animations.remove(i);   //TODO Figure out if blank anims make sense
    }
    recalc();
    updateSceneBounds();
}

bool Sheet::hasSelectedFrames()
{
    foreach(Animation* anim, animations)
    {
        if(anim->hasSelected())
            return true;
    }
    return false;
}

bool Sheet::selected(QGraphicsItem* it)
{
    foreach(Animation* anim, animations)
    {
        if(anim->isSelected(it))
            return true;
    }
    return false;
}

void Sheet::selectLine(QGraphicsItem* from, QGraphicsItem* to)
{
    bool selecting = false;
    foreach(Animation* anim, animations)
    {
        foreach(Frame* f, anim->getFrames())
        {
            if(f->isThis(to) && !selecting)
            {
                QGraphicsItem* tmp = to;
                to = from;
                from = tmp;
            }
            if(f->isThis(from))
                selecting = true;
            if(selecting)
            {
                if(!f->isSelected())
                    f->selectToggle();
            }
            if(f->isThis(to))
                return;
        }
    }
}

QLine Sheet::getDragPos(int x, int y)
{
    int curY = 0;
    Animation* over = NULL;
    for(int i = 0; i < animations.size(); i++)
    {
        Animation* anim = animations.at(i);
        curY += anim->getCurHeight();
        if(curY > y || i >= animations.size()-1)
        {
            over = anim;
            break;
        }
    }

    if(over)
        return over->getDragPos(x,y);

    return QLine(-1,-1,-1,-1);
}

void Sheet::dropped(int x, int y)
{
    int curY = 0;
    Animation* over = NULL;
    int overIndex = 0;
    foreach(Animation* anim, animations)
    {
        curY += anim->getCurHeight();
        if(curY > y || overIndex >= animations.size()-1)
        {
            over = anim;
            break;
        }
        overIndex++;
    }

    if(over)
    {
        //Get the position to drop
        int location = over->getDropPos(x, y);

        //Break out if there's no drop position
        if(location == ANIM_NONE)
            return;

        //Pull frames from animations
        QVector<Frame*> pulledFrames;
        foreach(Animation* anim, animations)
        {
            QVector<Frame*> frames = anim->pullSelected((anim == over)?(&location):(NULL));
            foreach(Frame* f, frames)
                pulledFrames.append(f);
        }

        //Figure out what to do with them
        if(location >= 0)                   //Add to this anim
        {
            over->addImages(pulledFrames, location);
        }
        else if(location == ANIM_BEFORE)    //Add before this anim
        {
            Animation* anim = new Animation(transparentBg, scene);
            anim->addImages(pulledFrames, 0);
            addAnimation(anim, overIndex);
        }
        else if(location == ANIM_AFTER)     //Add after this anim
        {
            Animation* anim = new Animation(transparentBg, scene);
            anim->addImages(pulledFrames, 0);
            addAnimation(anim, overIndex + 1);
        }

        //Delete animations that are empty as a result of this
        deleteEmpty();

        //Recalculate sheet positions
        refresh();
    }
}

void Sheet::deleteEmpty()
{
    for(int i = 0; i < animations.size(); i++)
    {
        if(animations.at(i)->isEmpty())
        {
            delete animations.at(i);
            animations.remove(i);
            i--;
        }
    }
}

void Sheet::deselectAll()
{
    foreach(Animation* anim, animations)
        anim->deselectAll();
}

bool Sheet::saveToStream(QDataStream& s)
{
    //Save sheet frames
    int curAnim = 0, curFrame = 0;

    s << animations.size();
    foreach(Animation* anim, animations)
    {
        QVector<Frame*> frames = anim->getFrames();
        s << frames.size();
        foreach(Frame* f, frames)
        {
            QByteArray imgByteArray;
            QBuffer buffer(&imgByteArray);
            buffer.open(QIODevice::WriteOnly);
            f->getImage()->save(&buffer, "PNG");
            s << imgByteArray;
            s << f->isSelected();
        }
    }

    //TODO Save anim names/labels
    s << animations.size(); //TODO Don't also have to save this
    for(int i = 0; i < animations.size(); i++)
        s << QString();

    //Save other stuff
    s << sheetBgCol;
    s << frameBgCol;
    s << QColor(255,255,255);   //TODO Font color
    s << frameBgTransparent << sheetBgTransparent;
    s << xSpacing << ySpacing << (int)width;
    s << QFont().toString();  //TODO Sheet font
    s << curAnim << curFrame;
    s << true;//ui->animNameEnabled->isChecked();   //TODO Anim names enabled

    return (s.status() == QDataStream::Ok);
}

bool Sheet::exportImage(QString sImgFilename)
{
    //TODO
    return true;
}

void Sheet::clear()
{
    foreach(Animation* animation, animations)
        delete animation;
    animations.clear();
    refresh();
}
