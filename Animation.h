#ifndef ANIMATION_H
#define ANIMATION_H
#include <QList>
#include <QImage>
#include <QColor>
#include <QString>
#include <QFont>

class Animation
{
protected:
    QList<QImage*> frames;
    int maxWidth;
    int sizeX, sizeY;
    QImage* curRender;
    QString title;
    QFont font;

public:
    Animation();

    int width();
    int height();
    void setMaxWidth(int width);

    void setTitle(QString str);
    void setFont(QFont f);
    void setBgTransparent(bool bTrans);
    void setBgFill(QColor col);
    void setSheetFill(QColor col);
    void setSheetTransparent(bool bTrans);
    void setSelectedCol(QColor col);
    void setFrameSelectCol(QColor col);

    void redraw();
    void reverse();
    void removeDuplicates();
    QImage* getImage();

    void addFrame(QImage* img, int pos = 0);
    QImage* getFrame(int pos);
    void deleteFrame(int pos);

    int getFrame(int x, int y);
};

#endif // ANIMATION_H
