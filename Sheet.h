#ifndef SHEET_H
#define SHEET_H

#include <QObject>
#include <QVector>
#include <QGraphicsRectItem>
#include <QRectF>
#include <QColor>
#include <QImage>
#include <QFont>
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
    QImage transparentBg;
    unsigned int xSpacing, ySpacing, dragRectWidth;
    SheetEditorView* sheetPreview;
    QFont font;
    QColor fontColor;
    int curSelectedAnim;
    bool animNamesVisible;

    void updateAnimBg();
public:
    explicit Sheet(QGraphicsScene* s, SheetEditorView* sheetView, QImage bg, unsigned int dragW, QObject *parent = 0);
    ~Sheet();

    void addAnimation(Animation* anim);
    void addAnimation(Animation* anim, unsigned int index);

    void setWidth(unsigned int w);
    void setBgCol(QColor c);
    QColor getBgCol() {return sheetBgCol;}
    void setFrameBgCol(QColor c);
    QColor getFrameBgCol() {return frameBgCol;}
    void setBgTransparent(bool b);
    void setFrameBgTransparent(bool b);

    void setXSpacing(unsigned int x);
    void setYSpacing(unsigned int y);

    QGraphicsScene* getScene() {return scene;}
    QImage getTransparentBg() {return transparentBg;}

    unsigned int getWidth() {return width;}
    unsigned int getHeight() {return curHeight;}
    void updateSceneBounds();
    unsigned int size() {return animations.size();}
    Animation* getAnimation(unsigned int index);   //Return NULL or the current animation
    QVector<Animation*>* getAnimationPtr() {return &animations;}
    int getOver(int x, int y);  //Get animation this xy position is over
    void refresh(){setWidth(width);updateSceneBounds();} //Recalculate sheet
    unsigned int getMinWidth(); //Get the minimum width for the current width
    unsigned int getSmallestPossibleWidth();    //Get the smallest possible width for this sheet (width of largest animation frame image)
    bool clicked(int x, int y, QGraphicsItem* it);
    bool hasSelectedFrames();     //Return true if there are selected frames in this sheet
    bool selected(QGraphicsItem* it);   //Return true if this item is selected
    void selectLine(QGraphicsItem* from, QGraphicsItem* to);
    QLine getDragPos(int x, int y);
    void dropped(int x, int y);
    void deselectAll();

    bool saveToStream(QDataStream& s);  //Save sheet to data stream
    void clear();   //Clears out this animation
    bool render(QString filename);    //Render this out as an image
    QFont getFont() {return font;}
    void setFont(QFont& f);
    void setFontColor(QColor col);
    QColor getFontColor() {return fontColor;}
    int getSelected(int x, int y);
    void selectAnimation(int selected);
    int getCurSelected();
    void setNamesVisible(bool b);
    QVector<int> deleteEmpty(); //Delete empty animations
    void removeAnimation(int idx);

signals:

public slots:
};

#endif // SHEET_H
