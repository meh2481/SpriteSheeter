#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "importdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void setImportImg(QString s);

public slots:
    void importNext(int numx, int numy);
    void importAll(int numx, int numy);

private slots:
    void on_openImagesButton_clicked();

private:
    Ui::MainWindow *ui;
    importDialog *mImportWindow;
    QStringList mOpenFiles;
    QString curImportImage;

    QImage* mCurSheet;
    QList<QList<QImage> > mSheetFrames;

    QGraphicsScene* animScene;
    QGraphicsPixmapItem* animItem;
    QGraphicsScene* sheetScene;
    QGraphicsPixmapItem* sheetItem;

    void openImportDiag();
    void importImage(QString s, int numxframes, int numyframes);
    void CenterParent(QWidget* parent, QWidget* child);

    void drawAnimation();
    void drawSheet();
};

#endif // MAINWINDOW_H
