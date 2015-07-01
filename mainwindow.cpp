#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDesktopWidget>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mImportWindow = new importDialog(this);

    QObject::connect(mImportWindow, SIGNAL(importOK(int, int)), this, SLOT(importNext(int, int)));
    QObject::connect(mImportWindow, SIGNAL(importAll(int, int)), this, SLOT(importAll(int, int)));
    QObject::connect(this, SIGNAL(setImportImg(QString)), mImportWindow, SLOT(setPreviewImage(QString)));
}

MainWindow::~MainWindow()
{
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

    QString windowTitle = "Animation for image ";
    mImportWindow->setModal(true);
    mImportWindow->setWindowTitle(windowTitle + curImportImage);
    setImportImg(curImportImage);

    mImportWindow->show();
    //Center on parent
    CenterParent(this, mImportWindow);
}

void MainWindow::importImage(QString s, int numxframes, int numyframes)
{

}

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












