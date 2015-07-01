#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mImportWindow = new importDialog(this);
}

MainWindow::~MainWindow()
{
    delete mImportWindow;
    delete ui;
}

void MainWindow::on_openImagesButton_clicked()
{
    //QStringList files = QFileDialog::getOpenFileNames(this, "Open Images", "", "All Files (*.*)");

    mImportWindow->setModal(true);
    //mImportWindow->setFixedSize(500,350);
    mImportWindow->show();
}
