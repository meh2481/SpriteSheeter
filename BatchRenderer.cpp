#include "BatchRenderer.h"
#include <QFontMetrics>
#include <QPainter>
#include <QRect>
#include <QTime>
#include <QCoreApplication>
#include <QDir>

BatchRenderer::BatchRenderer(QObject *parent) : QObject(parent)
{
}

void BatchRenderer::run()
{
    renderingStart(folder + ".png");

    //Load anim names from subfolder names, sheet frames from those
    QDir dir(folder);
    QStringList filters;
    filters << "*";
    QStringList animationDirs = dir.entryList(filters, QDir::Dirs | QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    foreach(QString f, animationDirs)
    {
        QList<QImage> animImages;
        mAnimNames.append(f);
        QString animFolder = folder + '/' + f;
        QDir animDir(animFolder);
        QStringList frameFilenames = animDir.entryList(filters, QDir::Files | QDir::NoSymLinks, QDir::Name | QDir::IgnoreCase);
        foreach(QString frameImgFilename, frameFilenames)
        {
            QString frameFilename = animFolder + '/' + frameImgFilename;
            QImage frameImg(frameFilename);
            if(!frameImg.isNull())
                animImages.append(frameImg);
        }

        if(animImages.size())
            mSheetFrames.append(animImages);
    }

    //Draw!
    QFontMetrics fm(sheetFont);
    float textHeight = fm.height() + 3;

    //First pass: Figure out dimensions of final image
    int iSizeX = offsetX;
    int iSizeY = offsetY;
    QStringList::iterator sName = mAnimNames.begin();
    foreach(QList<QImage> ql, mSheetFrames)
    {
        int ySize = 0;
        int iCurSizeX = offsetX;
        foreach(QImage img, ql)
        {
            //Test to see if we should start next line
            if(iCurSizeX + img.width() + offsetX > maxSheetWidth)
            {
                iSizeY += offsetY + ySize;
                ySize = 0;
                if(iCurSizeX > iSizeX)
                    iSizeX = iCurSizeX;
                iCurSizeX = offsetX;
            }

            if(img.height() > ySize)
                ySize = img.height();

            iCurSizeX += img.width() + offsetX;
        }

        iSizeY += offsetY + ySize;
        if(sName->length() && animNameEnabled)
            iSizeY += textHeight;
        sName++;

        if(iCurSizeX > iSizeX)
            iSizeX = iCurSizeX;
    }

    //Create sheet
    mCurSheet = new QImage(iSizeX, iSizeY, QImage::Format_ARGB32);

    //Create image of the proper size and fill it with a good bg color
    QPainter painter(mCurSheet);
    painter.setFont(sheetFont);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    if(sheetBgTransparent)
    {
        mCurSheet->fill(QColor(0,0,0,0));
    }
    else
        mCurSheet->fill(sheetBgCol);

    //Second pass: Print each frame into the final image
    int curX = offsetX;
    int curY = offsetY;
    sName = mAnimNames.begin();
    for(QList<QList<QImage> >::iterator ql = mSheetFrames.begin(); ql != mSheetFrames.end(); ql++)
    {
        QRect r;
        r.setRect(0,curY-offsetY,iSizeX,0);
        int ySize = 0;

        //Draw label for animation
        if(sName->length() && animNameEnabled)
        {
            painter.setPen(QColor(255,255,255,255));
            painter.drawText(QRectF(offsetX,curY,1000,textHeight), Qt::AlignLeft|Qt::AlignVCenter, *sName);
            curY += textHeight;
        }
        sName++;

        for(QList<QImage>::iterator img = ql->begin(); img != ql->end(); img++)
        {
            //Test to see if we should start next line
            if(curX + img->width() + offsetX > maxSheetWidth)
            {
                curY += offsetY + ySize;
                ySize = 0;
                curX = offsetX;
            }

            //Erase this portion of the image
            if(!frameBgTransparent)
            {
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                painter.fillRect(curX, curY, img->width(), img->height(), QBrush(frameBgCol));
            }

            painter.drawImage(curX, curY, *img);

            if(img->height() > ySize)
                ySize = img->height();
            curX += img->width() + offsetX;
        }
        curY += offsetY + ySize;
        curX = offsetX;
        r.setBottom(curY-offsetY);
    }
    painter.end();

    //Save image
    mCurSheet->save(folder + ".png", "PNG");

    //Clean up
    delete mCurSheet;

    //Emit a signal saying we're done bro
    renderingDone();
}
















