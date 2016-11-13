#include "Sheet.h"

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
    currentAnimation = 0;
}

Sheet::~Sheet()
{
    foreach(Animation* animation, animations)
        delete animation;
    scene->clear(); //Handles cleaning up all the memories
}

void Sheet::addAnimation(Animation* anim, unsigned int index)
{
    if(index > animations.size())
        index = animations.size();
    animations.insert(index, anim);
    anim->setSpacing(xSpacing, ySpacing);
    anim->setFrameBgCol(frameBgCol);
    anim->setFrameBgTransparent(frameBgTransparent);
    anim->setFrameBgVisible(!frameBgTransparent || !sheetBgTransparent);
    recalc();
    updateSceneBounds();
    currentAnimation = index;
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
    if(!animations.size())
        return; //Nothing to do TODO See if this is needed when removing anims

    unsigned int curY = 0;
    foreach(Animation* anim, animations)
    {
        anim->setOffset(0, curY);
        curY += anim->setWidth(width);
    }
    sceneRect.setBottom(curY);
    sceneRect.setRight(width);

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

void Sheet::reverseCurrentAnimation()
{
    if(currentAnimation < animations.size())
        animations.at(currentAnimation)->reverse();
}

bool Sheet::removeDuplicateFrames()
{
    if(currentAnimation < animations.size())
    {
        if(animations.at(currentAnimation)->removeDuplicateFrames())
        {
            recalc();
            updateSceneBounds();
            return true;
        }
    }
    return false;
}

Animation* Sheet::getCurAnimation()
{
    if(currentAnimation < animations.size())
        return animations.at(currentAnimation);
    return NULL;
}
