#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
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
    void animUpdate();

private slots:
    void on_openImagesButton_clicked();

    void on_xSpacingBox_valueChanged(int arg1);

    void on_ySpacingBox_valueChanged(int arg1);

    void on_saveSheetButton_clicked();

    void on_removeAnimButton_clicked();

    void on_animationNameEditor_textChanged(const QString &arg1);

    void on_prevAnimButton_clicked();

    void on_nextAnimButton_clicked();

    void on_animPlayButton_clicked();

    void on_animPauseButton_clicked();

    void on_animStopButton_clicked();

    void on_animPrevFrameButton_clicked();

    void on_animNextFrameButton_clicked();

    void on_animationSpeedSpinbox_valueChanged(int arg1);

    void on_openStripButton_clicked();

private:
    Ui::MainWindow *ui;
    importDialog *mImportWindow;
    QStringList mOpenFiles;
    QString curImportImage;
    QString lastSaveStr;

    QImage* mCurSheet;
    QList<QList<QImage> > mSheetFrames;
    QList<QList<QImage> >::iterator mCurAnim;
    QList<QImage>::iterator mCurFrame;
    QList<QString> mAnimNames;
    QList<QString>::iterator mCurAnimName;

    QGraphicsScene* animScene;
    QGraphicsPixmapItem* animItem;
    QGraphicsScene* sheetScene;
    QGraphicsPixmapItem* sheetItem;

    QTimer* animUpdateTimer;

    void openImportDiag();
    void importImage(QString s, int numxframes, int numyframes);
    void CenterParent(QWidget* parent, QWidget* child);

    void drawAnimation();
    void drawSheet(bool bHighlight = true);
};

#endif // MAINWINDOW_H
