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
    unsigned int size() {return animations.size();}
    Animation* getAnimation(unsigned int index);   //Return NULL or the current animation
    void refresh(){setWidth(width);updateSceneBounds();} //Recalculate sheet
    unsigned int getMinWidth(); //Get the minimum width for the current width
    unsigned int getSmallestPossibleWidth();    //Get the smallest possible width for this sheet (width of largest animation frame image)
    void clicked(int x, int y, QGraphicsItem* it);
    void deleteSelected();  //Delete currently selected frames/animations
    bool hasSelectedFrames();     //Return true if there are selected frames in this sheet
    bool selected(QGraphicsItem* it);   //Return true if this item is selected
    QLine getDragPos(int x, int y);
signals:

public slots:
};

#endif // SHEET_H
