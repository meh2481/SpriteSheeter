#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QColorDialog>
#include "ImportDialog.h"
#include "BalanceSheetDialog.h"
#include "IconExportDialog.h"
#include "SheetEditorView.h"
#include "RecentDocuments.h"
#include <QStack>
#include <QTextStream>
#include <QProgressDialog>

#define DRAG_HANDLE_SIZE 5
#define SCENE_BOUNDS    300

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define REV_VERSION   0

#define CURSOR_SZ 64

#define UNTITLED_IMAGE_STR "Untitled"

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
    bool setImportImg(QString s);
    void setBalanceDefWH(int w, int h);
    void setIconImage(QImage img);

public slots:
    void importNext(int numx, int numy, bool bVert, bool bSplit);
    void importAll(int numx, int numy, bool bVert, bool bSplit);
    void animUpdate();
    void mouseCursorPos(int x, int y);
    void mouseDown(int x, int y);
    void mouseUp(int x, int y);
    void newFile();
    void saveFileAs();
    void enableShortcuts(bool b);
    void undo();
    void redo();
    void saveSheet(QString filename = QString());
    void loadSheet(QString openFilename = QString());
    void addImages(QStringList l);
    void addFolders(QStringList l);
    void balance(int w, int h, BalanceSheetDialog::Pos vert, BalanceSheetDialog::Pos horiz);

    void startedBatchRender(QString sheetName);
    void finishedBatchRender();
    void threadRenderingCanceled();

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
    void on_saveFrameButton_clicked();
    void on_frameBgColSelect_clicked();
    void on_sheetBgColSelect_clicked();
    void on_FrameBgTransparent_toggled(bool checked);
    void on_SheetBgTransparent_toggled(bool checked);
    void on_balanceAnimButton_clicked();
    void on_fontButton_clicked();
    void on_xSpacingBox_editingFinished();
    void on_ySpacingBox_editingFinished();
    void on_sheetWidthBox_editingFinished();
    void on_animationNameEditor_editingFinished();
    void on_animNameEnabled_toggled(bool checked);
    void on_ExportAnimButton_clicked();
    void on_reverseAnimButton_clicked();
    void on_removeDuplicateFramesButton_clicked();
    void on_actionAbout_triggered();
    void on_actionBatch_Processing_triggered();

private:
    Ui::MainWindow *ui;
    ImportDialog *mImportWindow;
    BalanceSheetDialog *mBalanceWindow;
    IconExportDialog   *mIconExportWindow;
    RecentDocuments    *mRecentDocuments;
    QStringList mOpenFiles;
    QString curImportImage;
    QString lastSaveStr;
    QString lastImportExportStr;

    //Variables for drawing the current sheet/animation
    QImage* mCurSheet;
    QFont   sheetFont;
    QList<QList<QImage> > mSheetFrames;
    QList<QList<QImage> >::iterator mCurAnim;
    QList<QList<QImage> >::iterator m_selDragToAnim;
    QList<QImage>::iterator m_selDragToPos;
    QList<QImage>::iterator mCurFrame;
    QList<QImage>::iterator mCurSelected;
    QList<QList<QImage> >::iterator mCurSelectedInAnim;
    bool m_bDraggingSelected;
    bool m_bSetDraggingCursor;
    QRect m_rLastDragHighlight;
    bool m_bLastDragInAnim;
    QList<QString> mAnimNames;
    QList<QString>::iterator mCurAnimName;
    QImage* transparentBg;
    QColor sheetBgCol;
    QColor frameBgCol;
    QColor animHighlightCol;

    QStack<QByteArray*> undoList;
    QStack<QByteArray*> redoList;

    QString lastIconStr;
    QString lastOpenDir;
    QString lastGIFStr;

    QColorDialog colorSelect;

    int curMouseX;
    int curMouseY;

    bool bFileModified;
    bool bShortcuts;
    QString sCurFilename;

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
    bool bLoadMutex;

    //For clicking to select an animation
    QList<QRect> mAnimRects;

    QProgressDialog* progressBar;







    void openImportDiag();
    void importImage(QString s, int numxframes, int numyframes, bool bVert, bool bSplit);
    void importImageList(QStringList& fileList, QString prepend = QString(""), QString animName = QString(""));
    void CenterParent(QWidget* parent, QWidget* child);

    void drawAnimation();
    void drawSheet(bool bHighlight = true);

    void closeEvent(QCloseEvent *event);
    void readSettings();
    void cleanMemory();

    void insertAnimHelper(QList<QImage> imgList, QString name);

    void fixWindowTitle();
    void genUndoState();
    void pushUndo();
    void clearUndo();
    void clearRedo();
    void updateUndoRedoMenu();  //Update the menu icons to active/inactive as needed

    void genericSave(QString saveFilename);
    void saveToStream(QDataStream& s);
    void loadFromStream(QDataStream& s);

    bool loadAnimatedGIF(QString sFilename);    //Returns false on non-multi-page gif or failure, true on success

protected:
    //void changeEvent(QEvent *e);
    bool eventFilter(QObject* obj, QEvent *event);

};

#endif // MAINWINDOW_H
