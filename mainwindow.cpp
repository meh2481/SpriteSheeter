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
        mSheetFrames.push_back(imgList);
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

void MainWindow::drawSheet()
{
    if(!mSheetFrames.size())
        return;

    //First pass: Figure out dimensions of final image
    int offsetX = ui->xSpacingBox->value();
    int offsetY = ui->ySpacingBox->value();
    int iSizeX = offsetX;
    int iSizeY = 0;
    foreach(QList<QImage> ql, mSheetFrames)
    {
        int ySize = 0;
        int iCurSizeX = offsetX;
        foreach(QImage img, ql)
        {
            ySize = img.height();
            iCurSizeX += img.width() + offsetX;
        }
        iSizeY += offsetY + ySize;
        if(iCurSizeX > iSizeX)
            iSizeX = iCurSizeX;
    }

    if(mCurSheet)
        delete mCurSheet;

    //Create image of the proper size and fill it with a good bg color
    mCurSheet = new QImage(iSizeX, iSizeY, QImage::Format_RGB32);
    mCurSheet->fill(QColor(0, 128, 128, 255));

    //Second pass: Print each frame into the final image
    int curX = offsetX;
    int curY = offsetY / 2;
    QPainter painter(mCurSheet);
    foreach(QList<QImage> qil, mSheetFrames)
    {
        int ySize = 0;
        foreach(QImage img, qil)
        {
            painter.eraseRect(curX, curY, img.width(), img.height());
            //TODO: Specify bg color so we can fill this however we like
            painter.drawImage(curX, curY, img);
            ySize = img.height();
            curX += img.width() + offsetX;
        }
        curY += offsetY + ySize;
        curX += offsetX;
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





























