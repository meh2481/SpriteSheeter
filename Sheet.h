#ifndef SHEET_H
#define SHEET_H

#include <QObject>
#include <QVector>
#include <QGraphicsRectItem>
#include <QRectF>
#include <QColor>
#include <QImage>
#include "Animation.h"
#include "SheetEditorView.h"

class Sheet : public QObject
{
    Q_OBJECT

    QVector<Animation*> animations;
    QGraphicsScene* scene;

    unsigned int width, curHeight;
    unsigned int currentAnimation;

    Sheet(){}   //Private default constructor
    void recalc();

    QGraphicsRectItem* backgroundRect;  //Graphics item for background color
    QGraphicsRectItem* dragRect;        //Graphics item for drag area on the right

    QRectF sceneRect;
    QColor sheetBgCol;
    QColor frameBgCol;
    bool sheetBgTransparent;
    bool frameBgTransparent;
    QImage* transparentBg;
    unsigned int xSpacing, ySpacing, dragRectWidth;
    SheetEditorView* sheetPreview;

    void updateAnimBg();
public:
    explicit Sheet(QGraphicsScene* s, SheetEditorView* sheetView, QImage* bg, unsigned int dragW, QObject *parent = 0);
    ~Sheet();

    void addAnimation(Animation* anim);
    void addAnimation(Animation* anim, unsigned int index);

    void setWidth(unsigned int w);
    void setBgCol(QColor c);
    void setFrameBgCol(QColor c);
    void setBgTransparent(bool b);
    void setFrameBgTransparent(bool b);

    void setXSpacing(unsigned int x);
    void setYSpacing(unsigned int y);

    unsigned int getWidth() {return width;}
    unsigned int getHeight() {return curHeight;}
    void updateSceneBounds();
    void reverseCurrentAnimation();
    bool removeDuplicateFrames();   //TODO determine if should be in the context of current animation, or sheet
    unsigned int size() {return animations.size();}
    Animation* getCurAnimation();   //Return NULL or the current animation

signals:

public slots:
};

#endif // SHEET_H
