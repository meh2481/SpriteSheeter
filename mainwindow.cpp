#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDesktopWidget>
#include <QFile>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mImportWindow = new importDialog(this);

    QObject::connect(mImportWindow, SIGNAL(importOK(int, int)), this, SLOT(importNext(int, int)));
    QObject::connect(mImportWindow, SIGNAL(importAll(int, int)), this, SLOT(importAll(int, int)));
    QObject::connect(this, SIGNAL(setImportImg(QString)), mImportWindow, SLOT(setPreviewImage(QString)));

    animItem = NULL;
    animScene = NULL;
    sheetItem = NULL;
    sheetScene = NULL;
    mCurSheet = NULL;
    mCurAnim = mSheetFrames.begin();
    mCurAnimName = mAnimNames.begin();

    //Create gfx stuff for drawing to image contexts
    animScene = new QGraphicsScene();
    sheetScene = new QGraphicsScene();
    ui->animationPreview->setScene(animScene);
    ui->sheetPreview->setScene(sheetScene);
    ui->animationPreview->show();
    ui->sheetPreview->show();
}

MainWindow::~MainWindow()
{
    if(mCurSheet)
        delete mCurSheet;
    if(animItem)
        delete animItem;
    if(animScene)
        delete animScene;
    if(sheetItem)
        delete sheetItem;
    if(sheetScene)
        delete sheetScene;
    delete mImportWindow;
    delete ui;
}

void MainWindow::on_openImagesButton_clicked()
{
    mOpenFiles = QFileDialog::getOpenFileNames(this, "Open Images", "", "All Files (*.*)");

    openImportDiag();
}

void MainWindow::importNext(int numx, int numy)
{
    importImage(curImportImage, numx, numy);
    openImportDiag();   //Next one
}

void MainWindow::importAll(int numx, int numy)
{
    importImage(curImportImage, numx, numy);
    foreach(QString s, mOpenFiles)
    {
        importImage(s, numx, numy);
    }

    curImportImage = "";
    mOpenFiles.clear();
}

void MainWindow::openImportDiag()
{
    if(!mOpenFiles.size())
        return;

    curImportImage = mOpenFiles.takeFirst();    //Pop first filename and use it

    //Strip the file path from the image name
    int last = curImportImage.lastIndexOf("/");
    if(last == -1)
        last = curImportImage.lastIndexOf("\\");
    QString s = curImportImage;
    if(last != -1)
        s.remove(0, last+1);

    QString windowTitle = "Animation for image " + s;
    mImportWindow->setModal(true);
    mImportWindow->setWindowTitle(windowTitle);
    setImportImg(curImportImage);

    mImportWindow->show();
    //Center on parent
    CenterParent(this, mImportWindow);
}

void MainWindow::importImage(QString s, int numxframes, int numyframes)
{
    QImage image(s);
    if(image.isNull())
    {
        QMessageBox::information(this,"Image Import","Error opening image " + s);
        return;
    }

    //Find image dimensions
    int iXFrameSize = image.width() / numxframes;
    int iYFrameSize = image.height() / numyframes;

    //Grab all the frames out
    QList<QImage> imgList;
    for(int y = 0; y < numyframes; y++)
    {
        for(int x = 0; x < numxframes; x++)
        {
            imgList.push_back(image.copy(x*iXFrameSize, y*iYFrameSize, iXFrameSize, iYFrameSize));
        }
    }
    if(imgList.size())
    {
        //mSheetFrames.push_back(imgList);
        QList<QList<QImage> >::iterator it = mCurAnim;
        if(it != mSheetFrames.end())
            it++;
        mCurAnim = mSheetFrames.insert(it, imgList);

        QList<QString>::iterator itN = mCurAnimName;
        if(itN != mAnimNames.end())
            itN++;
        mCurAnimName = mAnimNames.insert(itN, QString(""));

        ui->animationNameEditor->setText(QString(""));
    }

    drawSheet();
}

//Example from http://www.qtforum.org/article/28852/center-any-child-window-center-parent.html
void MainWindow::CenterParent(QWidget* parent, QWidget* child)
{
    QPoint centerparent(
    parent->x() + ((parent->frameGeometry().width() - child->frameGeometry().width()) /2),
    parent->y() + ((parent->frameGeometry().height() - child->frameGeometry().height()) /2));

    QDesktopWidget * pDesktop = QApplication::desktop();
    QRect sgRect = pDesktop->screenGeometry(pDesktop->screenNumber(parent));
    QRect childFrame = child->frameGeometry();

    if(centerparent.x() < sgRect.left())
        centerparent.setX(sgRect.left());
    else if((centerparent.x() + childFrame.width()) > sgRect.right())
        centerparent.setX(sgRect.right() - childFrame.width());

    if(centerparent.y() < sgRect.top())
        centerparent.setY(sgRect.top());
    else if((centerparent.y() + childFrame.height()) > sgRect.bottom())
        centerparent.setY(sgRect.bottom() - childFrame.height());

    child->move(centerparent);
}

void MainWindow::drawAnimation()
{
    /*if(animItem == NULL)
    {
        animItem = new QGraphicsPixmapItem(QPixmap::fromImage(image));
        animScene->addItem(animItem);
    }
    else
        animItem->setPixmap(QPixmap::fromImage(image));*/
}

void MainWindow::drawSheet(bool bHighlight)
{
    float textHeight = 20;

    //First pass: Figure out dimensions of final image
    int offsetX = ui->xSpacingBox->value();
    int offsetY = ui->ySpacingBox->value();
    int iSizeX = offsetX;
    int iSizeY = offsetY;
    QList<QString>::iterator sName = mAnimNames.begin();
    int hiliteH = 0;
    foreach(QList<QImage> ql, mSheetFrames)
    {
        int ySize = 0;
        int iCurSizeX = offsetX;
        foreach(QImage img, ql)
        {
            ySize = img.height();
            iCurSizeX += img.width() + offsetX;
        }

        if(bHighlight && sName == mCurAnimName)
            hiliteH = ySize;
        sName++;

        iSizeY += offsetY + ySize + textHeight;
        if(iCurSizeX > iSizeX)
            iSizeX = iCurSizeX;
    }

    if(mCurSheet)
        delete mCurSheet;

    //Create image of the proper size and fill it with a good bg color
    mCurSheet = new QImage(iSizeX, iSizeY, QImage::Format_ARGB32);
    mCurSheet->fill(QColor(0, 128, 128, 255));

    //Second pass: Print each frame into the final image
    int curX = offsetX;
    int curY = offsetY;
    QPainter painter(mCurSheet);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    sName = mAnimNames.begin();
    foreach(QList<QImage> ql, mSheetFrames)
    {
        int ySize = 0;

        //Highlight our current anim red
        if(bHighlight && sName == mCurAnimName)
            painter.fillRect(0, curY-offsetY, mCurSheet->width(), hiliteH + textHeight + offsetY*2, QColor(128,0,0,255));

        //Draw label for animation
        painter.drawText(QRectF(offsetX,curY,1000,textHeight), Qt::AlignLeft|Qt::AlignVCenter, *sName);
        sName++;
        curY += textHeight;

        foreach(QImage img, ql)
        {
            //Erase this portion of the image
            painter.fillRect(curX, curY, img.width(), img.height(), Qt::transparent);

            //TODO: Specify bg color so we can fill this however we like
            painter.drawImage(curX, curY, img);
            ySize = img.height();
            curX += img.width() + offsetX;
        }
        curY += offsetY + ySize;
        curX = offsetX;
    }
    painter.end();


    //Update the GUI to show this image
    if(sheetItem == NULL)
    {
        sheetItem = new QGraphicsPixmapItem(QPixmap::fromImage(*mCurSheet));
        sheetScene->addItem(sheetItem);
    }
    else
        sheetItem->setPixmap(QPixmap::fromImage(*mCurSheet));
    ui->sheetPreview->show();
}

//Redraw the sheet if either of these change
void MainWindow::on_xSpacingBox_valueChanged(int arg1)
{
    drawSheet();
}

void MainWindow::on_ySpacingBox_valueChanged(int arg1)
{
    drawSheet();
}

void MainWindow::on_saveSheetButton_clicked()
{
    if(!mCurSheet) return;

    drawSheet(false);   //Save a non-highlighted version

    QString sSel = "PNG Image (*.png)";
    QString saveFilename = QFileDialog::getSaveFileName(this,
                                                        tr("Save Spritesheet"),
                                                        lastSaveStr,
                                                        tr("PNG Image (*.png);;Windows Bitmap (*.bmp);;TIFF Image(*.tiff)"),
                                                        &sSel);

    if(saveFilename.length())
    {
        lastSaveStr = saveFilename;
        if(!mCurSheet->save(saveFilename))
        {
            QMessageBox::information(this,"Image Export","Error saving image " + saveFilename);
        }
    }
    drawSheet(true);    //Redraw the highlighted version
}

void MainWindow::on_removeAnimButton_clicked()
{
    //mSheetFrames.pop_back();    //TODO: Remove current
    if(mCurAnim != mSheetFrames.end())
    {
        mCurAnim = mSheetFrames.erase(mCurAnim);
        if(mCurAnim == mSheetFrames.end() && mCurAnim != mSheetFrames.begin())
            mCurAnim--;
    }
    if(mCurAnimName != mAnimNames.end())
    {
        mCurAnimName = mAnimNames.erase(mCurAnimName);
        if(mCurAnimName == mAnimNames.end() && mCurAnimName != mAnimNames.begin())
            mCurAnimName--;
    }

    drawSheet();

    if(mCurAnimName != mAnimNames.end())
        ui->animationNameEditor->setText(*mCurAnimName);
    else
        ui->animationNameEditor->setText(QString(""));
}

void MainWindow::on_animationNameEditor_textChanged(const QString &arg1)
{
    if(mCurAnimName != mAnimNames.end())
        *mCurAnimName = arg1;
    drawSheet();
}

void MainWindow::on_prevAnimButton_clicked()
{
    if(mCurAnim != mSheetFrames.begin())
        mCurAnim--;
    if(mCurAnimName != mAnimNames.begin())
        mCurAnimName--;

    drawSheet();

    if(mCurAnimName != mAnimNames.end())
        ui->animationNameEditor->setText(*mCurAnimName);
    else
        ui->animationNameEditor->setText(QString(""));
}

void MainWindow::on_nextAnimButton_clicked()
{
    if(mCurAnim != mSheetFrames.end())
    {
        mCurAnim++;
        if(mCurAnim == mSheetFrames.end())
            mCurAnim--;
    }
    if(mCurAnimName != mAnimNames.end())
    {
        mCurAnimName++;
        if(mCurAnimName == mAnimNames.end())
            mCurAnimName--;
    }

    drawSheet();

    if(mCurAnimName != mAnimNames.end())
        ui->animationNameEditor->setText(*mCurAnimName);
    else
        ui->animationNameEditor->setText(QString(""));
}



























