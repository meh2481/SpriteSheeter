#ifndef ANIMATION_H
#define ANIMATION_H
#include <QList>
#include <QImage>
#include <QColor>
#include <QString>
#include <QFont>
#include <QRect>

#define FONT_SPACING 3

class Animation
{
protected:
    QList<QImage*> frames;
    QList<QRect> frameRects;
    int maxWidth;
    int sizeX, sizeY;
    int spaceX, spaceY;
    QImage* curRender;
    QString title;
    bool titleEnabled;
    QFont font;
    bool bgTrans;
    bool sheetTrans;
    QColor bgFill;
    QColor sheetFill;
    QColor selectedCol;
    QColor frameSelectedCol;
    QImage bgImage; //Image to draw if bg is transparent

    void recalculate();

public:
    Animation();

    int width();
    int height();
    void setMaxWidth(int width);

    void setTitle(QString str);
    void setFont(QFont f);
    void setBgTransparent(bool bTrans);
    void setBgImage(QImage bg);
    void setBgFill(QColor col);
    void setSheetFill(QColor col);
    void setSheetTransparent(bool bTrans);
    void setSelectedCol(QColor col);
    void setFrameSelectCol(QColor col);
    void setSpacing(int x, int y);
    void setNameEnabled(bool enabled);

    void redraw();
    void reverse();
    bool removeDuplicates();    //Return true if there were duplicates, false otherwise
    QImage* getImage();

    void addFrame(QImage* img, int pos = -1);
    QImage* getFrame(int pos);
    void deleteFrame(int pos);
    void mouseOverFrame(int pos);
    void selectFrame(int pos);

    void draggingTo(int x, int y);  //Dragging to here; draw some sort of visual cue
    void droppedAt(int x, int y, QList<QImage*> images);    //Dropped these images here; add them

    void balance(/*TODO*/);
    void saveAs(QString GIFFilename);

    int getFrame(int x, int y);
};

#endif // ANIMATION_H





























