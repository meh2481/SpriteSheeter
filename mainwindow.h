#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "importdialog.h"
#include "sheeteditorview.h"

#define DRAG_HANDLE_SIZE 5

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void keyPressEvent(QKeyEvent* e);

signals:
    void setImportImg(QString s);

public slots:
    void importNext(int numx, int numy, bool bVert);
    void importAll(int numx, int numy, bool bVert);
    void animUpdate();
    void mouseCursorPos(int x, int y);
    void mouseDown(int x, int y);
    void mouseUp(int x, int y);
    void newFile();
    void saveFile();
    void addImages(QStringList l);
    void addFolders(QStringList l);

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
    void on_sheetWidthBox_valueChanged(int arg1);

private:
    Ui::MainWindow *ui;
    importDialog *mImportWindow;
    QStringList mOpenFiles;
    QString curImportImage;
    QString lastSaveStr;

    //Variables for drawing the current sheet/animation
    QImage* mCurSheet;
    QList<QList<QImage> > mSheetFrames;
    QList<QList<QImage> >::iterator mCurAnim;
    QList<QImage>::iterator mCurFrame;
    QList<QImage>::iterator mCurSelected;
    QList<QList<QImage> >::iterator mCurSelectedInAnim;
    QList<QString> mAnimNames;
    QList<QString>::iterator mCurAnimName;

    int curMouseX;
    int curMouseY;

    //Variables for dealing with the Qt draw engine
    QGraphicsScene* animScene;
    QGraphicsPixmapItem* animItem;
    QGraphicsScene* msheetScene;
    QGraphicsPixmapItem* sheetItem;

    //Animation update timer
    QTimer* animUpdateTimer;

    //Having to do with clicking & dragging the right side of a sheet
    int mStartSheetW;
    int xStartDragSheetW;
    bool bDraggingSheetW;

    //For clicking to select an animation
    QList<QRect> mAnimRects;

    void openImportDiag();
    void importImage(QString s, int numxframes, int numyframes, bool bVert);
    void importImageList(QStringList& fileList, QString prepend = QString(""), QString animName = QString(""));
    void CenterParent(QWidget* parent, QWidget* child);

    void drawAnimation();
    void drawSheet(bool bHighlight = true);

    void closeEvent(QCloseEvent *event);
    void readSettings();
};

#endif // MAINWINDOW_H
