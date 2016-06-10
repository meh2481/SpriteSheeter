#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ZoomableGraphicsView.h"
#include "SheetEditorView.h"
#include <QFileDialog>
#include <QDesktopWidget>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QFontDialog>
#include <QFontMetrics>
#include <QBuffer>
#include "FreeImage.h"
#include <string.h>
#include <QListView>
#include <QTreeView>
#include "BatchRenderer.h"
#include <QThreadPool>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mImportWindow = new ImportDialog(this);
    mBalanceWindow = new BalanceSheetDialog(this);
    mIconExportWindow = new IconExportDialog(this);
    mRecentDocuments = new RecentDocuments(this);
    mRecentDocuments->init(ui->menuFile, ui->actionQuit);

    //Connect all our signals & slots up
    QObject::connect(mImportWindow, SIGNAL(importOK(int, int, bool, bool)), this, SLOT(importNext(int, int, bool, bool)));
    QObject::connect(mImportWindow, SIGNAL(importAll(int, int, bool, bool)), this, SLOT(importAll(int, int, bool, bool)));
    QObject::connect(this, SIGNAL(setImportImg(QString)), mImportWindow, SLOT(setPreviewImage(QString)));
    QObject::connect(ui->sheetPreview, SIGNAL(mouseMoved(int,int)), this, SLOT(mouseCursorPos(int, int)));
    QObject::connect(ui->sheetPreview, SIGNAL(mousePressed(int,int)), this, SLOT(mouseDown(int, int)));
    QObject::connect(ui->sheetPreview, SIGNAL(mouseReleased(int,int)), this, SLOT(mouseUp(int, int)));
    QObject::connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(on_saveSheetButton_clicked()));
    QObject::connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(saveFileAs()));
    QObject::connect(ui->actionImport_WIP_Sheet, SIGNAL(triggered(bool)), this, SLOT(loadSheet()));
    QObject::connect(ui->actionUndo, SIGNAL(triggered(bool)), this, SLOT(undo()));
    QObject::connect(ui->actionRedo, SIGNAL(triggered(bool)), this, SLOT(redo()));
    QObject::connect(ui->actionEnableShortcuts, SIGNAL(toggled(bool)), this, SLOT(enableShortcuts(bool)));
    QObject::connect(ui->sheetPreview, SIGNAL(droppedFiles(QStringList)), this, SLOT(addImages(QStringList)));
    QObject::connect(ui->sheetPreview, SIGNAL(droppedFolders(QStringList)), this, SLOT(addFolders(QStringList)));
    QObject::connect(mBalanceWindow, SIGNAL(balance(int,int,BalanceSheetDialog::Pos,BalanceSheetDialog::Pos)), this, SLOT(balance(int,int,BalanceSheetDialog::Pos,BalanceSheetDialog::Pos)));
    QObject::connect(this, SIGNAL(setBalanceDefWH(int,int)), mBalanceWindow, SLOT(defaultWH(int,int)));
    QObject::connect(this, SIGNAL(setIconImage(QImage)), mIconExportWindow, SLOT(setImage(QImage)));
    QObject::connect(mRecentDocuments, SIGNAL(openFile(QString)), this, SLOT(loadSheet(QString)));

    animItem = NULL;
    animScene = NULL;
    sheetItem = NULL;
    msheetScene = NULL;
    mCurSheet = NULL;
    transparentBg = new QImage("://bg");
    mCurAnim = mSheetFrames.begin();
    mCurAnimName = mAnimNames.begin();
    bShortcuts = true;
    bLoadMutex = false;

    bDraggingSheetW = false;
    m_bDraggingSelected = false;
    m_bSetDraggingCursor = false;
    bFileModified = false;
    m_rLastDragHighlight.setCoords(0,0,0,0);
    m_bLastDragInAnim = false;
    sCurFilename = UNTITLED_IMAGE_STR;
    //TODO Store initial undo state
    pushUndo();
    updateUndoRedoMenu();

    animUpdateTimer = new QTimer(this);
    QObject::connect(animUpdateTimer, SIGNAL(timeout()), this, SLOT(animUpdate()));

    //Create gfx stuff for drawing to image contexts
    animScene = new QGraphicsScene();
    msheetScene = new QGraphicsScene();
    ui->animationPreview->setScene(animScene);
    ui->sheetPreview->setScene(msheetScene);
    ui->animationPreview->show();
    ui->sheetPreview->show();
    ui->animationNameEditor->installEventFilter(this);

    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 0);

    ZoomableGraphicsView* z = new ZoomableGraphicsView(ui->sheetPreview);
    z->set_modifiers(Qt::NoModifier);

    z = new ZoomableGraphicsView(ui->animationPreview);
    z->set_modifiers(Qt::NoModifier);

    sheetBgCol = QColor(0, 128, 128, 255);
    frameBgCol = QColor(0, 255, 0, 255);
    animHighlightCol = QColor(128, 0, 0, 255);

    //Read in settings here
    readSettings();

    //Set color icons to proper color
    QPixmap colIcon(32, 32);
    colIcon.fill(sheetBgCol);
    QIcon ic(colIcon);
    ui->sheetBgColSelect->setIcon(ic);

    colIcon.fill(frameBgCol);
    QIcon ic2(colIcon);
    ui->frameBgColSelect->setIcon(ic2);

    fixWindowTitle();
}

MainWindow::~MainWindow()
{
    clearUndo();
    clearRedo();
    if(transparentBg)
        delete transparentBg;
    if(mCurSheet)
        delete mCurSheet;
    if(animItem)
        delete animItem;
    if(animScene)
        delete animScene;
    if(sheetItem)
        delete sheetItem;
    if(msheetScene)
        delete msheetScene;
    delete mIconExportWindow;
    delete animUpdateTimer;
    delete mImportWindow;
    delete mBalanceWindow;
    delete ui;
}

void MainWindow::addImages(QStringList l)
{
    mOpenFiles = l;
    openImportDiag();
}

void MainWindow::importImageList(QStringList& fileList, QString prepend, QString animName)
{
    QList<QImage> imgList;
    foreach(QString s1, fileList)
    {
        QString imgPath = prepend + s1;
        QImage image(imgPath);
        if(!image.isNull())
            imgList.push_back(image);
    }
    if(imgList.size())
    {
        QList<QList<QImage> >::iterator it = mCurAnim;
        if(it != mSheetFrames.end())
            it++;
        mCurAnim = mSheetFrames.insert(it, imgList);

        QList<QString>::iterator itN = mCurAnimName;
        if(itN != mAnimNames.end())
            itN++;
        mCurAnimName = mAnimNames.insert(itN, animName);

        ui->animationNameEditor->setText(animName);
    }

    if(mCurAnim != mSheetFrames.end())
        mCurFrame = mCurAnim->begin();

    drawSheet();
    drawAnimation();
    genUndoState();
}

void MainWindow::addFolders(QStringList l)
{
    foreach(QString s, l)
    {
        QDir folder(s);
        QStringList fileFilters;
        //Filter out only image files
        //TODO: Rather than specifying manually, remove filter entirely or add ALL actual file types we support
        fileFilters << "*.png" << "*.dds" << "*.bmp" << "*.gif" << "*.pbm" << "*.pgm" << "*.ppm" << "*.tif" << "*.tiff" << "*.xbm" << "*.xpm" << "*.tga" << "*.jpg" << "*.jpeg";
        QStringList files = folder.entryList(fileFilters, QDir::Files, QDir::Name); //Get list of all files in this folder

        importImageList(files, s + '/', folder.dirName());
    }
}

void MainWindow::on_openImagesButton_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Import Image as Animation Sheet", lastOpenDir, "All Files (*.*)");
    if(files.size())
    {
        QString s = (*files.begin());
        QFileInfo inf(s);
        lastOpenDir = inf.absoluteDir().absolutePath();
    }
    addImages(files);
}

void MainWindow::on_openStripButton_clicked()
{
    mOpenFiles = QFileDialog::getOpenFileNames(this, "Import Frame Sequence", lastOpenDir, "All Files (*.*)");
    if(mOpenFiles.size())
    {
        QString s = (*mOpenFiles.begin());
        QFileInfo inf(s);
        lastOpenDir = inf.absoluteDir().absolutePath();
    }
    importImageList(mOpenFiles);
}

void MainWindow::importNext(int numx, int numy, bool bVert, bool bSplit)
{
    importImage(curImportImage, numx, numy, bVert, bSplit);
    openImportDiag();   //Next one
}

void MainWindow::importAll(int numx, int numy, bool bVert, bool bSplit)
{
    importImage(curImportImage, numx, numy, bVert, bSplit);
    foreach(QString s, mOpenFiles)
    {
        importImage(s, numx, numy, bVert, bSplit);
    }

    curImportImage = "";
    mOpenFiles.clear();
}

void MainWindow::openImportDiag()
{
    if(!mOpenFiles.size())
        return;

    curImportImage = mOpenFiles.takeFirst();    //Pop first filename and use it

    //Test for/filter out animated .gif
    if(curImportImage.endsWith(".gif", Qt::CaseInsensitive))
    {
        if(loadAnimatedGIF(curImportImage)) //If this fails, fall through and treat this image as per normal
        {
            openImportDiag();   //Recursively call so we get the next image
            return;             //Break out here so we don't add it accidentally
        }
    }

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
    if(setImportImg(curImportImage))
    {
        mImportWindow->show();
        //Center on parent
        CenterParent(this, mImportWindow);
    }
}

void MainWindow::insertAnimHelper(QList<QImage> imgList, QString name)
{
    if(imgList.size())
    {
        QList<QList<QImage> >::iterator it = mCurAnim;
        if(it != mSheetFrames.end())
            it++;
        mCurAnim = mSheetFrames.insert(it, imgList);

        QList<QString>::iterator itN = mCurAnimName;
        if(itN != mAnimNames.end())
            itN++;

        mCurAnimName = mAnimNames.insert(itN, name);

        ui->animationNameEditor->setText(name);
    }
}

void MainWindow::importImage(QString s, int numxframes, int numyframes, bool bVert, bool bSplit)
{
    QImage image(s);
    if(image.isNull())
    {
        QMessageBox::information(this,"Image Import","Error opening image " + s);
        return;
    }
    QString fileName = QFileInfo(s).baseName();

    //Find image dimensions
    int iXFrameSize = image.width() / numxframes;
    int iYFrameSize = image.height() / numyframes;

    //Grab all the frames out
    QList<QImage> imgList;
    if(!bVert)
    {
        for(int y = 0; y < numyframes; y++)
        {
            for(int x = 0; x < numxframes; x++)
            {
                imgList.push_back(image.copy(x*iXFrameSize, y*iYFrameSize, iXFrameSize, iYFrameSize));
            }
            if(bSplit)
            {
                insertAnimHelper(imgList, fileName + '_' + QString::number(y));
                imgList.clear();
            }
        }
    }
    else
    {
        for(int x = 0; x < numxframes; x++)
        {
            for(int y = 0; y < numyframes; y++)
            {
                imgList.push_back(image.copy(x*iXFrameSize, y*iYFrameSize, iXFrameSize, iYFrameSize));
            }
            if(bSplit)
            {
                insertAnimHelper(imgList, fileName + '_' + QString::number(x));
                imgList.clear();
            }
        }
    }
    if(!bSplit)
        insertAnimHelper(imgList, fileName);

    if(mCurAnim != mSheetFrames.end())
        mCurFrame = mCurAnim->begin();

    drawSheet();
    drawAnimation();
    genUndoState();
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

void MainWindow::drawSheet(bool bHighlight)
{
    QFontMetrics fm(sheetFont);
    float textHeight = fm.height() + 3;
    int maxSheetWidth = ui->sheetWidthBox->value();

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
            //Test to see if we should start next line
            if(iCurSizeX + img.width() + offsetX > maxSheetWidth)
            {
                iSizeY += offsetY + ySize;
                if(bHighlight && sName == mCurAnimName)
                    hiliteH += ySize + offsetY;
                ySize = 0;
                if(iCurSizeX > iSizeX)
                    iSizeX = iCurSizeX;
                iCurSizeX = offsetX;
            }

            if(img.height() > ySize)
                ySize = img.height();

            iCurSizeX += img.width() + offsetX;
        }

        if(bHighlight && sName == mCurAnimName)
            hiliteH += ySize;

        iSizeY += offsetY + ySize;
        if(sName->length() && ui->animNameEnabled->isChecked())
            iSizeY += textHeight;
        sName++;

        if(iCurSizeX > iSizeX)
            iSizeX = iCurSizeX;
    }

    if(bHighlight)
        iSizeX += DRAG_HANDLE_SIZE;

    //Only recreate sheet if we have to
    if(mCurSheet && (iSizeX != mCurSheet->width() || iSizeY != mCurSheet->height()))
    {
        delete mCurSheet;
        mCurSheet = new QImage(iSizeX, iSizeY, QImage::Format_ARGB32);
    }
    else if(!mCurSheet)
        mCurSheet = new QImage(iSizeX, iSizeY, QImage::Format_ARGB32);

    //Create image of the proper size and fill it with a good bg color
    QPainter painter(mCurSheet);
    painter.setFont(sheetFont);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    if(ui->SheetBgTransparent->isChecked() && bHighlight)
    {
        QBrush bgTexBrush(*transparentBg);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.fillRect(0, 0, iSizeX, iSizeY, bgTexBrush);
    }
    else if(ui->SheetBgTransparent->isChecked())
    {
        mCurSheet->fill(QColor(0,0,0,0));
    }
    else
        mCurSheet->fill(sheetBgCol);

    //Second pass: Print each frame into the final image
    int curX = offsetX;
    int curY = offsetY;
    sName = mAnimNames.begin();
    mAnimRects.clear();
    for(QList<QList<QImage> >::iterator ql = mSheetFrames.begin(); ql != mSheetFrames.end(); ql++)
    {
        QRect r;
        r.setRect(0,curY-offsetY/2.0,iSizeX,0);
        int ySize = 0;

        //Highlight our current anim red
        if(bHighlight && sName == mCurAnimName)
        {
            if(sName->length() && ui->animNameEnabled->isChecked())
                painter.fillRect(0, curY-offsetY/2.0, mCurSheet->width(), hiliteH + textHeight + offsetY, animHighlightCol);
            else
                painter.fillRect(0, curY-offsetY/2.0, mCurSheet->width(), hiliteH + offsetY, animHighlightCol);
        }

        //Draw label for animation
        if(sName->length() && ui->animNameEnabled->isChecked())
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
            if(bHighlight && ui->FrameBgTransparent->isChecked())
            {
                QBrush bgTexBrush(*transparentBg);
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                painter.fillRect(curX, curY, img->width(), img->height(), bgTexBrush);
            }
            else if(!ui->FrameBgTransparent->isChecked())
            {
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                painter.fillRect(curX, curY, img->width(), img->height(), QBrush(frameBgCol));
            }

            painter.drawImage(curX, curY, *img);

            //If we're highlighting this image, draw blue overtop
            if(bHighlight && mCurSelected == img)
            {
                painter.fillRect(curX, curY, img->width(), img->height(), QBrush(QColor(0,0,255,100)));
                painter.setCompositionMode(QPainter::CompositionMode_Source);
            }

            if(img->height() > ySize)
                ySize = img->height();
            curX += img->width() + offsetX;
        }
        curY += offsetY + ySize;
        curX = offsetX;
        r.setBottom(curY-offsetY/2.0);
        mAnimRects.push_back(r);
    }

    //Draw drag handle on right side
    if(bHighlight)
        painter.fillRect(iSizeX - DRAG_HANDLE_SIZE, 0, DRAG_HANDLE_SIZE, mCurSheet->height(), QColor(0,230,230,255));

    painter.end();


    //Update the GUI to show this image
    msheetScene->clear();
    sheetItem = new QGraphicsPixmapItem(QPixmap::fromImage(*mCurSheet));
    msheetScene->addItem(sheetItem);

    //Set the new rect of the scene
    //Scale based on minimum scene bounds, and current viewport aspect ratio
    int scene_bounds = SCENE_BOUNDS;
    if(scene_bounds < mCurSheet->width()/2.0)
        scene_bounds = mCurSheet->width()/2.0;
    if(scene_bounds < mCurSheet->height()/2.0)
        scene_bounds = mCurSheet->height()/2.0;
    float hFac = (float)ui->sheetPreview->width()/(float)ui->sheetPreview->height();
    msheetScene->setSceneRect(-scene_bounds*hFac, -scene_bounds, mCurSheet->width()+scene_bounds*2*hFac, mCurSheet->height()+scene_bounds*2);

    if(ui->sheetPreview->isHidden())
        ui->sheetPreview->show();
}

//Redraw the sheet if either of these change
void MainWindow::on_xSpacingBox_valueChanged(int arg1)
{
    //drawSheet();
    Q_UNUSED(arg1);
}

void MainWindow::on_ySpacingBox_valueChanged(int arg1)
{
    //drawSheet();
    Q_UNUSED(arg1);
}

void MainWindow::genericSave(QString saveFilename)
{
    if(saveFilename.length())
    {
        drawSheet(false);   //Save a non-highlighted version
        lastSaveStr = saveFilename;
        if(saveFilename.contains(".sheet", Qt::CaseInsensitive))
        {
            saveSheet(saveFilename);
            QFileInfo fi(saveFilename);
            sCurFilename = fi.fileName();
            bFileModified = false;
            fixWindowTitle();
            //TODO Store file orig state
        }
        else
        {
            if(!mCurSheet->save(saveFilename))
            {
                QMessageBox::information(this,"Image Export","Error saving image " + saveFilename);
            }
            else
            {
                QFileInfo fi(saveFilename);
                sCurFilename = fi.fileName();
                bFileModified = false;
                fixWindowTitle();
                //TODO Store file orig state
            }
        }
        drawSheet(true);    //Redraw the highlighted version
    }
}

//Save file
void MainWindow::on_saveSheetButton_clicked()
{
    if(!mCurSheet || !mSheetFrames.size()) return;

    if(!bFileModified) return;  //Don't bother saving if we already have

    QString saveFilename;
    if(sCurFilename == UNTITLED_IMAGE_STR)  //Haven't saved this yet
    {
        QString sSel;
        if(lastSaveStr.contains(".sheet", Qt::CaseInsensitive))
            sSel = "Sprite Sheet (*.sheet)";
        else if(lastSaveStr.contains(".bmp", Qt::CaseInsensitive))
            sSel = "Windows Bitmap (*.bmp)";
        else if(lastSaveStr.contains(".tiff", Qt::CaseInsensitive))
            sSel = "TIFF Image (*.tiff)";
        else
            sSel = "PNG Image (*.png)"; //Default to PNG the first time they use

        saveFilename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Sheet"),
                                                    lastSaveStr,
                                                    tr("PNG Image (*.png);;Windows Bitmap (*.bmp);;TIFF Image (*.tiff);;Sprite Sheet (*.sheet)"),
                                                    &sSel);
    }
    else
        saveFilename = lastSaveStr;

    genericSave(saveFilename);
}

void MainWindow::saveFileAs()
{
    if(!mCurSheet || !mSheetFrames.size()) return;

    QString sSel;
    if(lastSaveStr.contains(".sheet", Qt::CaseInsensitive))
        sSel = "Sprite Sheet (*.sheet)";
    else if(lastSaveStr.contains(".bmp", Qt::CaseInsensitive))
        sSel = "Windows Bitmap (*.bmp)";
    else if(lastSaveStr.contains(".tiff", Qt::CaseInsensitive))
        sSel = "TIFF Image (*.tiff)";
    else
        sSel = "PNG Image (*.png)"; //Default to PNG the first time they use

    QString saveFilename = QFileDialog::getSaveFileName(this,
                                                        tr("Save Sheet As"),
                                                        lastSaveStr,
                                                        tr("PNG Image (*.png);;Windows Bitmap (*.bmp);;TIFF Image (*.tiff);;Sprite Sheet (*.sheet)"),
                                                        &sSel);

    genericSave(saveFilename);
}

void MainWindow::on_removeAnimButton_clicked()
{
    if(mCurAnim == mSheetFrames.end() || mCurAnimName == mAnimNames.end())
        return;

    mCurAnim = mSheetFrames.erase(mCurAnim);
    if(mCurAnim == mSheetFrames.end() && mCurAnim != mSheetFrames.begin())
        mCurAnim--;

    mCurAnimName = mAnimNames.erase(mCurAnimName);
    if(mCurAnimName == mAnimNames.end() && mCurAnimName != mAnimNames.begin())
        mCurAnimName--;

    drawSheet();

    if(mCurAnimName != mAnimNames.end())
        ui->animationNameEditor->setText(*mCurAnimName);
    else
        ui->animationNameEditor->setText(QString(""));

    if(mCurAnim != mSheetFrames.end())
        mCurFrame = mCurAnim->begin();

    drawAnimation();
    genUndoState();
}

void MainWindow::on_animationNameEditor_textChanged(const QString &arg1)
{
    if(mCurAnimName != mAnimNames.end())
        *mCurAnimName = arg1;
    //drawSheet();
}

void MainWindow::on_prevAnimButton_clicked()
{
    //Move these back one
    if(mCurAnim == mSheetFrames.begin() || mCurAnimName == mAnimNames.begin())
        return;

    QList<QImage> curAnim = *mCurAnim;
    mCurAnim = mSheetFrames.erase(mCurAnim);
    mCurAnim--;
    mCurAnim = mSheetFrames.insert(mCurAnim, curAnim);

    QString curName = *mCurAnimName;
    mCurAnimName = mAnimNames.erase(mCurAnimName);
    mCurAnimName--;
    mCurAnimName = mAnimNames.insert(mCurAnimName, curName);


    drawSheet();

    if(mCurAnimName != mAnimNames.end())
        ui->animationNameEditor->setText(*mCurAnimName);
    else
        ui->animationNameEditor->setText(QString(""));

    if(mCurAnim != mSheetFrames.end())
        mCurFrame = mCurAnim->begin();

    drawAnimation();
    genUndoState();
}

void MainWindow::on_nextAnimButton_clicked()
{
    if(mCurAnim == mSheetFrames.end() || mCurAnimName == mAnimNames.end())
        return;

    QList<QImage> curAnim = *mCurAnim;
    mCurAnim = mSheetFrames.erase(mCurAnim);
    if(mCurAnim != mSheetFrames.end())
        mCurAnim++;
    mCurAnim = mSheetFrames.insert(mCurAnim, curAnim);

    QString curName = *mCurAnimName;
    mCurAnimName = mAnimNames.erase(mCurAnimName);
    if(mCurAnimName != mAnimNames.end())
        mCurAnimName++;
    mCurAnimName = mAnimNames.insert(mCurAnimName, curName);

    drawSheet();

    if(mCurAnimName != mAnimNames.end())
        ui->animationNameEditor->setText(*mCurAnimName);
    else
        ui->animationNameEditor->setText(QString(""));

    if(mCurAnim != mSheetFrames.end())
        mCurFrame = mCurAnim->begin();

    drawAnimation();
    genUndoState();
}

void MainWindow::drawAnimation()
{
    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size() || mCurFrame == mCurAnim->end())
    {
        if(animItem)
            animItem->hide();
        return;
    }

    //Draw image and bg
    QImage animFrame(mCurFrame->width(), mCurFrame->height(), QImage::Format_ARGB32);
    QPainter painter(&animFrame);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    if(ui->FrameBgTransparent->isChecked())
    {
        QBrush bgTexBrush(*transparentBg);
        painter.fillRect(0, 0, mCurFrame->width(), mCurFrame->height(), bgTexBrush);
    }
    else
        animFrame.fill(frameBgCol);

    painter.drawImage(0, 0, *mCurFrame);


    if(animItem == NULL)
    {
        animItem = new QGraphicsPixmapItem(QPixmap::fromImage(animFrame));
        animScene->addItem(animItem);
    }
    else
        animItem->setPixmap(QPixmap::fromImage(animFrame));

    animItem->show();
    animScene->setSceneRect(0, 0, animFrame.width(), animFrame.height());
}

void MainWindow::animUpdate()
{
    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
        return;

    if(mCurFrame == mCurAnim->end())
        mCurFrame = mCurAnim->begin();
    else
    {
        mCurFrame++;
        if(mCurFrame == mCurAnim->end())
            mCurFrame = mCurAnim->begin();
    }

    drawAnimation();
}

void MainWindow::on_animationSpeedSpinbox_valueChanged(int arg1)
{
    int iInterval = 1000/arg1;
    animUpdateTimer->stop();
    animUpdateTimer->start(iInterval);
}

void MainWindow::on_animPlayButton_clicked()
{
    int iInterval = 1000/ui->animationSpeedSpinbox->value();
    animUpdateTimer->start(iInterval);
}

void MainWindow::on_animPauseButton_clicked()
{
    animUpdateTimer->stop();
}

void MainWindow::on_animStopButton_clicked()
{
    animUpdateTimer->stop();

    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
        return;

    mCurFrame = mCurAnim->begin();
    drawAnimation();
}

void MainWindow::on_animPrevFrameButton_clicked()
{
    if(animUpdateTimer->isActive())
        animUpdateTimer->stop();

    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
        return;

    if(mCurFrame == mCurAnim->begin())
        mCurFrame = mCurAnim->end();
    mCurFrame--;

    drawAnimation();
}

void MainWindow::on_animNextFrameButton_clicked()
{
    if(animUpdateTimer->isActive())
        animUpdateTimer->stop();

    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
        return;

    mCurFrame++;
    if(mCurFrame == mCurAnim->end())
        mCurFrame = mCurAnim->begin();

    drawAnimation();
}

void MainWindow::mouseCursorPos(int x, int y)
{
    QFontMetrics fm(sheetFont);
    float textHeight = fm.height() + 3;

    if(mCurSheet)
    {
        //Update cursor if need be
        if(x <= mCurSheet->width() &&
           x >= mCurSheet->width() - DRAG_HANDLE_SIZE &&
           y <= mCurSheet->height() &&
           y >= 0 &&
           !m_bDraggingSelected)
        {
             ui->sheetPreview->setCursor(Qt::SizeHorCursor);
        }
        else if(!bDraggingSheetW && !m_bDraggingSelected)
            ui->sheetPreview->setCursor(Qt::ArrowCursor);

        //See if we're resizing the sheet
        if(bDraggingSheetW)
        {
            ui->sheetWidthBox->setValue(mStartSheetW - (xStartDragSheetW - x));
            drawSheet();
        }

    }
    statusBar()->showMessage(QString::number(x) + ", " + QString::number(y));

    int maxSheetWidth = ui->sheetWidthBox->value();
    int offsetX = ui->xSpacingBox->value();
    int offsetY = ui->ySpacingBox->value();
    int curX = offsetX;
    int curY = offsetY;
    if(!m_bDraggingSelected)    //If we're not dragging a frame currently
    {
        QList<QImage>::iterator mPrevSelected = mCurSelected;

        if(mCurAnim != mSheetFrames.end())
            mCurSelected = mCurAnim->end();

        int prevX, prevY, newX, newY;
        prevX = prevY = newX = newY = -1;

        QList<QString>::iterator sName = mAnimNames.begin();
        mCurSelectedInAnim = mSheetFrames.end();
        for(QList<QList<QImage> >::iterator ql = mSheetFrames.begin(); ql != mSheetFrames.end(); ql++)
        {
            int ySize = 0;

            if(sName->length())
                curY += textHeight;
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

                //Found previous image; store draw coordinates
                if(img == mPrevSelected)
                {
                    prevX = curX;
                    prevY = curY;
                }

                //Check and see if we're overlapping this portion of the image
                //painter.fillRect(curX, curY, img.width(), img.height(), Qt::transparent);
                if(x >= curX && x < curX + img->width() &&
                   y >= curY && y < curY + img->height())
                {
                    mCurSelected = img;
                    mCurSelectedInAnim = ql;
                    newX = curX;
                    newY = curY;    //Store draw coordinates
                }

                if(img->height() > ySize)
                    ySize = img->height();
                curX += img->width() + offsetX;
            }

            curY += offsetY + ySize;
            curX = offsetX;
        }

        if(mPrevSelected != mCurSelected)
        {
            //Redraw both
            QPainter painter(mCurSheet);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            if(ui->FrameBgTransparent->isChecked())
            {
                QBrush bgTexBrush(*transparentBg);
                if(newX > 0)
                    painter.fillRect(newX, newY, mCurSelected->width(), mCurSelected->height(), bgTexBrush);
                if(prevX > 0)
                    painter.fillRect(prevX, prevY, mPrevSelected->width(), mPrevSelected->height(), bgTexBrush);
            }
            else
            {
                if(newX > 0)
                    painter.fillRect(newX, newY, mCurSelected->width(), mCurSelected->height(), QBrush(frameBgCol));
                if(prevX > 0)
                    painter.fillRect(prevX, prevY, mPrevSelected->width(), mPrevSelected->height(), QBrush(frameBgCol));
            }

            //Draw images back
            if(newX > 0)
                painter.drawImage(newX, newY, *mCurSelected);
            if(prevX > 0)
                painter.drawImage(prevX, prevY, *mPrevSelected);

            //If we're highlighting this image, draw blue overtop
            if(newX > 0)
                painter.fillRect(newX, newY, mCurSelected->width(), mCurSelected->height(), QBrush(QColor(0,0,255,100)));

            painter.end();

            //Update the GUI to show this image
            if(sheetItem != NULL)
                sheetItem->setPixmap(QPixmap::fromImage(*mCurSheet));
        }
    }
    else
    {
        //Currently dragging something
        if(!m_bSetDraggingCursor)
        {
            //Haven't set cursor yet; do so
            m_bSetDraggingCursor = true;
            QPixmap pMap;
            pMap.convertFromImage(*mCurSelected);
            if(pMap.width() > pMap.height() && pMap.width() > CURSOR_SZ)
                pMap = pMap.scaledToWidth(CURSOR_SZ, Qt::SmoothTransformation);
            else if(pMap.width() <= pMap.height() && pMap.height() > CURSOR_SZ)
                pMap = pMap.scaledToHeight(CURSOR_SZ, Qt::SmoothTransformation);
            ui->sheetPreview->setCursor(QCursor(pMap));
        }

        QList<QString>::iterator sName = mAnimNames.begin();
        m_selDragToAnim = mSheetFrames.end();
        QRect rcDraw;
        rcDraw.setCoords(0,0,0,0);
        bool bInHighlight = false;
        for(QList<QList<QImage> >::iterator ql = mSheetFrames.begin(); ql != mSheetFrames.end(); ql++)
        {
            int ySize = 0;
            if(sName->length())
            {
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

                //Check if inserting BEFORE first image of a row
                if(curX == offsetX &&
                   x < curX + img->width() / 2 &&
                   y >= curY - offsetY/2 &&
                   y < curY + img->height() + offsetY/2)
                {
                    m_selDragToPos = img;
                    m_selDragToAnim = ql;
                    rcDraw.setCoords(0, curY - offsetY/2.0, offsetX, curY + img->height() + offsetY/2.0);
                    if(ql == mCurAnim)
                        bInHighlight = true;
                }
                //Check and see if we're dragging PAST this image. If so, we'll insert AFTER it.
                else if(x >= curX + img->width() / 2 &&
                        y >= curY - offsetY/2 &&
                        y < curY + img->height() + offsetY/2)
                {
                    m_selDragToPos = img;
                    m_selDragToPos++;
                    m_selDragToAnim = ql;
                    rcDraw.setCoords(curX + img->width() - 1, curY - offsetY/2.0, curX + img->width() + offsetX, curY + img->height() + offsetY/2.0);
                    if(ql == mCurAnim)
                        bInHighlight = true;
                }

                if(img->height() > ySize)
                    ySize = img->height();
                curX += img->width() + offsetX;
            }

            curY += offsetY + ySize;
            curX = offsetX;
        }
        //if(rcDraw.width() == 0 && rcDraw.height() == 0) //Didn't hit anything
        {
            //Check and see if below image
            if(x >= 0 && x <= mCurSheet->width() && y > mCurSheet->height())
            {
                rcDraw.setCoords(0, mCurSheet->height() - offsetY - 1, mCurSheet->width(), mCurSheet->height());
            }
        }

        //See if we should draw handle showing where we'll insert this frame
        if(rcDraw != m_rLastDragHighlight)  //Update last drawing place
        {
            QPainter painter(mCurSheet);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

            if(rcDraw.width() && rcDraw.height())
            {
                //Draw new place, in blue (highlight color)
                painter.fillRect(rcDraw, QBrush(QColor(0,0,255,255)));
            }
            if(m_rLastDragHighlight.width() && m_rLastDragHighlight.height())
            {
                //Erase old place
                if(m_bLastDragInAnim)
                {
                    painter.fillRect(m_rLastDragHighlight, QBrush(animHighlightCol));
                }
                else if(ui->SheetBgTransparent->isChecked())
                {
                    QBrush bgTexBrush(*transparentBg);
                    painter.fillRect(m_rLastDragHighlight, bgTexBrush);
                }
                else
                {
                    painter.fillRect(m_rLastDragHighlight, QBrush(sheetBgCol));
                }
            }

            painter.end();

            //Update the GUI to show this image
            if(sheetItem != NULL)
                sheetItem->setPixmap(QPixmap::fromImage(*mCurSheet));
        }

        m_bLastDragInAnim = bInHighlight;
        m_rLastDragHighlight = rcDraw;
    }

    curMouseY = y;
    curMouseX = x;
}

void MainWindow::mouseDown(int x, int y)
{
    if(mCurSheet)
    {
        //We're starting to drag the sheet size handle
        if(x <= mCurSheet->width() &&
           x >= mCurSheet->width() - DRAG_HANDLE_SIZE &&
           y <= mCurSheet->height() &&
           y >= 0)
        {
            bDraggingSheetW = true;
            mStartSheetW = mCurSheet->width();
            xStartDragSheetW = x;
        }
        else
        {
            //Figure out what anim we're clicking on
            if(!mAnimRects.empty() && !mSheetFrames.empty())
            {
                QList<QList<QImage> >::iterator i = mSheetFrames.begin();
                QList<QString>::iterator name = mAnimNames.begin();
                foreach(QRect r, mAnimRects)
                {
                    if(r.contains(x,y))
                    {
                        mCurAnim = i;
                        mCurAnimName = name;

                        drawSheet();
                        if(mCurAnimName != mAnimNames.end())
                            ui->animationNameEditor->setText(*mCurAnimName);
                        else
                            ui->animationNameEditor->setText(QString(""));

                        if(mCurAnim != mSheetFrames.end())
                            mCurFrame = mCurAnim->begin();
                        drawAnimation();

                        break;
                    }
                    i++;
                    name++;
                }
            }

            //Start dragging if we should...
            if(mCurSelectedInAnim != mSheetFrames.end())
            {
                m_bDraggingSelected = true;
                m_bSetDraggingCursor = false;
                m_selDragToAnim = mSheetFrames.end();
                m_rLastDragHighlight.setCoords(0,0,0,0);
            }
        }
    }
}

void MainWindow::mouseUp(int x, int y)
{
    if(mCurSheet)
    {
        if(bDraggingSheetW)
        {
            ui->sheetWidthBox->setValue(mStartSheetW - (xStartDragSheetW - x));
            bDraggingSheetW = false;
            drawSheet();
            genUndoState();
        }

        if(m_bDraggingSelected)
        {
            m_bDraggingSelected = false;
            if(!mAnimRects.empty() && !mSheetFrames.empty())
            {
                QList<QList<QImage> >::iterator i = mSheetFrames.begin();
                QList<QString>::iterator name = mAnimNames.begin();
                bool bDropped = false;
                foreach(QRect r, mAnimRects)
                {
                    if(y >= r.top() && y <= r.bottom())//r.contains(x,y))
                    {
                        //Drop off in position...
                        if(m_selDragToAnim != mSheetFrames.end())
                        {
                            //If we're dragging within an animation, just swap positions
                            if(m_selDragToAnim == mCurSelectedInAnim)
                            {
                                int startpos = mCurSelected - mCurSelectedInAnim->begin();
                                int endpos = m_selDragToPos - m_selDragToAnim->begin();
                                if(startpos != endpos && startpos != endpos-1)  //Make sure we're not dragging into itself
                                {
                                    if(endpos < startpos)
                                        endpos++;
                                    if(endpos <= 0)
                                        m_selDragToAnim->move(startpos, 0);
                                    else
                                        m_selDragToAnim->move(startpos, endpos-1);
                                }
                                mCurFrame = mCurAnim->begin();
                            }
                            else    //Move to another anim
                            {
                                QImage img = *mCurSelected;
                                m_selDragToAnim->insert(m_selDragToPos, img);
                                mCurSelectedInAnim->erase(mCurSelected);
                                mCurAnim = mCurSelectedInAnim;
                                mCurFrame = mCurAnim->begin();
                                if(mCurFrame == mCurAnim->end())    //We erased this whole animation; clean up
                                    on_removeAnimButton_clicked();  //Simulate deleting this anim
                            }
                            genUndoState();
                        }

                        bDropped = true;
                        break;
                    }
                    i++;
                    name++;
                }

                if(!bDropped)
                {
                    if(y > mCurSheet->height())
                    {
                        //Create new anim for this frame
                        QImage img = *mCurSelected;
                        mCurSelectedInAnim->erase(mCurSelected);
                        mCurFrame = mCurAnim->begin();
                        if(mCurFrame == mCurAnim->end())
                        {
                            //We erased this whole animation; clean up
                            on_removeAnimButton_clicked();  //Simulate deleting this anim
                        }
                        QList<QImage> qNewAnim;
                        qNewAnim.push_back(img);
                        mSheetFrames.push_back(qNewAnim);
                        mAnimNames.push_back("");
                        mCurAnimName = mAnimNames.end();
                        mCurAnimName--;
                        mCurAnim = mSheetFrames.end();
                        mCurAnim--;
                        mCurFrame = mCurAnim->begin();
                        genUndoState();
                    }
                }
            }
            drawSheet();
            drawAnimation();
            mouseCursorPos(x,y);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //Make sure user has saved before closing
    if(bFileModified)
    {
        QMessageBox::StandardButton dialog;
        dialog = QMessageBox::warning(this, "Save Changes",
                                      "Do you want to save changes to \"" + sCurFilename + "\"?",
                                      QMessageBox::Discard | QMessageBox::Save | QMessageBox::Cancel);

        if(dialog == QMessageBox::Save)
        {
            on_saveSheetButton_clicked();
            if(bFileModified)   //If they still haven't saved...
            {
                event->ignore();
                return;
            }
        }
        else if(dialog != QMessageBox::Discard)
        {
            event->ignore();
            return;
        }
    }

    QSettings settings("DaxarDev", "SpriteSheeter");
    //Save window geometry
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    //Save other GUI settings
    settings.setValue("xSpacing", ui->xSpacingBox->value());
    settings.setValue("ySpacing", ui->ySpacingBox->value());
    settings.setValue("sheetWidth", ui->sheetWidthBox->value());
    settings.setValue("animationSpeed", ui->animationSpeedSpinbox->value());
    settings.setValue("FrameBgTransparent", ui->FrameBgTransparent->isChecked());
    settings.setValue("SheetBgTransparent", ui->SheetBgTransparent->isChecked());
    settings.setValue("sheetBgColr", sheetBgCol.red());
    settings.setValue("sheetBgColg", sheetBgCol.green());
    settings.setValue("sheetBgColb", sheetBgCol.blue());
    settings.setValue("frameBgColr", frameBgCol.red());
    settings.setValue("frameBgColg", frameBgCol.green());
    settings.setValue("frameBgColb", frameBgCol.blue());
    settings.setValue("lastSaveStr", lastSaveStr);
    settings.setValue("lastIconStr", lastIconStr);
    settings.setValue("lastOpenDir", lastOpenDir);
    settings.setValue("lastImportExportStr", lastImportExportStr);
    settings.setValue("sheetFont", sheetFont.toString());
    settings.setValue("shortcuts", bShortcuts);
    settings.setValue("animNames", ui->animNameEnabled->isChecked());
    settings.setValue("lastGIFStr", lastGIFStr);
    //settings.setValue("", );
    QMainWindow::closeEvent(event);
}

void MainWindow::readSettings()
{
    //Read in settings from config (registry or wherever it is)
    bLoadMutex = true;
    QSettings settings("DaxarDev", "SpriteSheeter");
    if(settings.value("xSpacing", -1).toInt() == -1)    //No settings are here
        return;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->xSpacingBox->setValue(settings.value("xSpacing").toInt());
    ui->ySpacingBox->setValue(settings.value("ySpacing").toInt());
    ui->sheetWidthBox->setValue(settings.value("sheetWidth").toInt());
    ui->animationSpeedSpinbox->setValue(settings.value("animationSpeed").toInt());
    ui->FrameBgTransparent->setChecked(settings.value("FrameBgTransparent").toBool());
    ui->SheetBgTransparent->setChecked(settings.value("SheetBgTransparent").toBool());
    sheetBgCol.setRed(settings.value("sheetBgColr").toInt());
    sheetBgCol.setGreen(settings.value("sheetBgColg").toInt());
    sheetBgCol.setBlue(settings.value("sheetBgColb").toInt());
    frameBgCol.setRed(settings.value("frameBgColr").toInt());
    frameBgCol.setGreen(settings.value("frameBgColg").toInt());
    frameBgCol.setBlue(settings.value("frameBgColb").toInt());
    lastSaveStr = settings.value("lastSaveStr").toString();
    lastIconStr = settings.value("lastIconStr").toString();
    lastOpenDir = settings.value("lastOpenDir").toString();
    lastImportExportStr = settings.value("lastImportExportStr").toString();
    QString sFontVal = settings.value("sheetFont").toString();
    if(!sFontVal.size())
        sheetFont = QFont("MS Shell Dlg 2", 8);
    else
        sheetFont.fromString(sFontVal);
    if(settings.value("shortcuts").isValid())
    {
        bShortcuts = settings.value("shortcuts").toBool();
        ui->actionEnableShortcuts->setChecked(bShortcuts);
    }
    if(settings.value("animNames").isValid())
        ui->animNameEnabled->setChecked(settings.value("animNames").toBool());
    if(settings.value("lastGIFStr").isValid())
        lastGIFStr = settings.value("lastGIFStr").toString();



    //Other initialization stuff!

    //Fill in frame/sheet colors
    QPixmap colIcon(32, 32);
    colIcon.fill(sheetBgCol);
    QIcon ic(colIcon);
    ui->sheetBgColSelect->setIcon(ic);
    colIcon.fill(frameBgCol);
    ic = QIcon(colIcon);
    ui->frameBgColSelect->setIcon(ic);

    ui->frameBgColSelect->setEnabled(!ui->FrameBgTransparent->isChecked());
    ui->sheetBgColSelect->setEnabled(!ui->SheetBgTransparent->isChecked());

    bLoadMutex = false;
}

void MainWindow::on_sheetWidthBox_valueChanged(int arg1)
{
    //drawSheet();
    Q_UNUSED(arg1);
}

void MainWindow::newFile()
{
    //Don't overwrite changes when they create a new file
    if(bFileModified)
    {
        QMessageBox::StandardButton dialog;
        dialog = QMessageBox::warning(this, "Save Changes",
                                      "Do you want to save changes to \"" + sCurFilename + "\"?",
                                      QMessageBox::Discard | QMessageBox::Save | QMessageBox::Cancel);

        if(dialog == QMessageBox::Save)
        {
            on_saveSheetButton_clicked();
            if(bFileModified)   //If they still haven't saved...
                return;
        }
        else if(dialog != QMessageBox::Discard)
            return;
    }

    if(mCurSheet)
        delete mCurSheet;
    mCurSheet = NULL;

    mSheetFrames.clear();
    mCurAnim = mSheetFrames.begin();
    mAnimNames.clear();
    mCurAnimName = mAnimNames.begin();

    drawSheet();
    drawAnimation();

    sCurFilename = UNTITLED_IMAGE_STR;
    bFileModified = false;
    fixWindowTitle();

    //TODO Store file orig state, clear undo/redo
    clearUndo();
    clearRedo();
    pushUndo();
    updateUndoRedoMenu();
}

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    //Deleting current selected frame
    if(e->key() == Qt::Key_Delete && mSheetFrames.size() && mCurAnim != mSheetFrames.end() && mCurSelected != mCurAnim->end() && mCurSelectedInAnim != mSheetFrames.end())
    {
        mCurSelectedInAnim->erase(mCurSelected);
        mCurSelected = mCurSelectedInAnim->end();

        //Test and see if this anim is now empty
        if(!mCurSelectedInAnim->size())
        {
            mCurAnim = mCurSelectedInAnim;
            on_removeAnimButton_clicked();  //Simulate deleting this anim
        }

        mouseCursorPos(curMouseX, curMouseY);   //Select again
        drawSheet();
        genUndoState();
    }

    //Shortcut keys if user enabled these
    if(bShortcuts)
    {
        if(e->key() == Qt::Key_W)
        {
            if(e->modifiers() & Qt::ShiftModifier)
            {
                on_prevAnimButton_clicked();
            }
            else
            {
                //Move back one anim
                if(mCurAnim != mSheetFrames.begin() && mCurAnimName != mAnimNames.begin())
                {
                    mCurAnim--;
                    mCurAnimName--;

                    drawSheet();

                    if(mCurAnimName != mAnimNames.end())
                        ui->animationNameEditor->setText(*mCurAnimName);
                    else
                        ui->animationNameEditor->setText(QString(""));

                    if(mCurAnim != mSheetFrames.end())
                        mCurFrame = mCurAnim->begin();

                    drawAnimation();
                }
            }
        }
        else if(e->key() == Qt::Key_S)
        {
            if(e->modifiers() & Qt::ShiftModifier)
            {
                on_nextAnimButton_clicked();
            }
            else
            {
                //Move forward one anim
                if(mCurAnim != mSheetFrames.end() && mCurAnimName != mAnimNames.end())
                {
                    mCurAnim++;
                    mCurAnimName++;
                    if(mCurAnim == mSheetFrames.end() || mCurAnimName == mAnimNames.end())
                    {
                        mCurAnim--;
                        mCurAnimName--;
                    }
                    else
                    {
                        drawSheet();

                        if(mCurAnimName != mAnimNames.end())
                            ui->animationNameEditor->setText(*mCurAnimName);
                        else
                            ui->animationNameEditor->setText(QString(""));

                        if(mCurAnim != mSheetFrames.end())
                            mCurFrame = mCurAnim->begin();

                        drawAnimation();
                    }
                }
            }
        }
        else if(e->key() == Qt::Key_Q)
        {
            on_openStripButton_clicked();
        }
        else if(e->key() == Qt::Key_A)
        {
            on_openImagesButton_clicked();
        }
        else if(e->key() == Qt::Key_E)
        {
            on_removeAnimButton_clicked();
        }
        else if(e->key() == Qt::Key_D)
        {
            on_saveSheetButton_clicked();
        }
    }
}

void MainWindow::on_saveFrameButton_clicked()
{
    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size() || mCurFrame == mCurAnim->end())
        return;

    setIconImage(*mCurFrame);
    mIconExportWindow->show();
}

bool MainWindow::eventFilter(QObject* obj, QEvent *event)
{
    if (obj == ui->animationNameEditor)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Up)
            {
                //Move back one anim
                if(mCurAnim != mSheetFrames.begin() && mCurAnimName != mAnimNames.begin())
                {
                    mCurAnim--;
                    mCurAnimName--;
                }

                drawSheet();

                if(mCurAnimName != mAnimNames.end())
                    ui->animationNameEditor->setText(*mCurAnimName);
                else
                    ui->animationNameEditor->setText(QString(""));

                if(mCurAnim != mSheetFrames.end())
                    mCurFrame = mCurAnim->begin();

                drawAnimation();

                ui->animationNameEditor->selectAll();
                genUndoState();
                return true;
            }
            else if(keyEvent->key() == Qt::Key_Down)
            {
                //Move forward one anim
                if(mCurAnim != mSheetFrames.end() && mCurAnimName != mAnimNames.end())
                {
                    mCurAnim++;
                    mCurAnimName++;
                    if(mCurAnim == mSheetFrames.end() || mCurAnimName == mAnimNames.end())
                    {
                        mCurAnim--;
                        mCurAnimName--;
                    }
                }

                drawSheet();

                if(mCurAnimName != mAnimNames.end())
                    ui->animationNameEditor->setText(*mCurAnimName);
                else
                    ui->animationNameEditor->setText(QString(""));

                if(mCurAnim != mSheetFrames.end())
                    mCurFrame = mCurAnim->begin();

                drawAnimation();

                ui->animationNameEditor->selectAll();
                genUndoState();
                return true;
            }
        }
        return false;
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::on_frameBgColSelect_clicked()
{
    QColor selected = colorSelect.getColor(frameBgCol, this, "Select Frame Background Color");
    if(selected.isValid())
    {
        frameBgCol = selected;
        QPixmap colIcon(32, 32);
        colIcon.fill(frameBgCol);
        QIcon ic(colIcon);
        ui->frameBgColSelect->setIcon(ic);
        drawSheet();
        drawAnimation();
        genUndoState();
    }
}

void MainWindow::on_sheetBgColSelect_clicked()
{
    QColor selected = colorSelect.getColor(sheetBgCol, this, "Select Sheet Background Color");
    if(selected.isValid())
    {
        sheetBgCol = selected;
        QPixmap colIcon(32, 32);
        colIcon.fill(sheetBgCol);
        QIcon ic(colIcon);
        ui->sheetBgColSelect->setIcon(ic);
        drawSheet();
        genUndoState();
    }
}

void MainWindow::on_FrameBgTransparent_toggled(bool checked)
{
    if(bLoadMutex) return;
    ui->frameBgColSelect->setEnabled(!checked);
    drawSheet();
    drawAnimation();
    genUndoState();
}

void MainWindow::on_SheetBgTransparent_toggled(bool checked)
{
    if(bLoadMutex) return;
    ui->sheetBgColSelect->setEnabled(!checked);
    drawSheet();
    genUndoState();
}

void MainWindow::on_balanceAnimButton_clicked()
{
    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size() || mCurFrame == mCurAnim->end())
        return;

    int w = 0, h = 0;
    foreach(QImage img, *mCurAnim)
    {
        if(img.width() > w)
            w = img.width();
        if(img.height() > h)
            h = img.height();
    }
    setBalanceDefWH(w, h);

    mBalanceWindow->show();
    CenterParent(this, mBalanceWindow);
}

void MainWindow::balance(int w, int h, BalanceSheetDialog::Pos vert, BalanceSheetDialog::Pos horiz)
{
    qDebug() << "enter function balance()" << endl;
    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
        return;

    QMutableListIterator<QImage> it(*mCurAnim);
    while(it.hasNext())
    {
        qDebug() << "balance() loop begin" << endl;
        QImage img = it.next();

        //Use vert/horiz
        int xPos, yPos;
        if(vert == BalanceSheetDialog::Up)
            yPos = 0;
        else if(vert == BalanceSheetDialog::Mid)
            yPos = (h/2)-(img.height()/2);
        else
            yPos = h - img.height();

        if(horiz == BalanceSheetDialog::Left)
            xPos = 0;
        else if(horiz == BalanceSheetDialog::Mid)
            xPos = (w/2)-(img.width()/2);
        else
            xPos = w - img.width();

        qDebug() << "balance() create final image" << endl;
        QImage final(w, h, QImage::Format_ARGB32);
        final.fill(QColor(0,0,0,0));
        QPainter painter(&final);
        qDebug() << "balance() draw new img size" << endl;
        painter.drawImage(xPos, yPos, img);
        painter.end();
        it.setValue(final);
     }

    mCurFrame = mCurAnim->begin();
    qDebug() << "balance() draw sheet" << endl;
    drawSheet();
    qDebug() << "balance() draw animation" << endl;
    drawAnimation();
    qDebug() << "balance() gen undo" << endl;
    genUndoState();
    qDebug() << "balance() end" << endl;
}

void MainWindow::undo()
{
    if(undoList.size() > 1)
    {
        bFileModified = true;
        //Save our redo point
        QByteArray* ba = undoList.pop();
        redoList.push(ba);

        //Undo
        cleanMemory();
        ba = undoList.top();
        QDataStream s(ba, QIODevice::ReadOnly);
        loadFromStream(s);

        updateUndoRedoMenu();
    }
}

void MainWindow::redo()
{
    if(redoList.size())
    {
        bFileModified = true;
        //Save this back on our undo list (top of undo list is current state)
        QByteArray* ba = redoList.pop();
        undoList.push(ba);

        //Load this state
        cleanMemory();
        QDataStream s(ba, QIODevice::ReadOnly);
        loadFromStream(s);

        updateUndoRedoMenu();
    }
}

void MainWindow::saveSheet(QString filename)
{
    if(!filename.size())
    {
        if(!mCurSheet || !mSheetFrames.size()) return;

        QString saveFilename = QFileDialog::getSaveFileName(this,
                                                            tr("Export WIP Sheet"),
                                                            lastImportExportStr,
                                                            tr("Sprite Sheet (*.sheet)"));
        filename = saveFilename;
    }
    if(filename.length())
    {
        lastImportExportStr = filename;
        mRecentDocuments->addDocument(filename);
        QFile f(filename);
        if(f.open(QIODevice::WriteOnly))
        {
            QDataStream s(&f);
            s.setVersion(QDataStream::Qt_5_4);

            saveToStream(s);
        }
    }
}

void MainWindow::loadSheet(QString openFilename)
{
    //Don't overwrite changes when they create a new file
    if(bFileModified)
    {
        QMessageBox::StandardButton dialog;
        dialog = QMessageBox::warning(this, "Save Changes",
                                      "Do you want to save changes to \"" + sCurFilename + "\"?",
                                      QMessageBox::Discard | QMessageBox::Save | QMessageBox::Cancel);

        if(dialog == QMessageBox::Save)
        {
            on_saveSheetButton_clicked();
            if(bFileModified)   //If they still haven't saved...
                return;
        }
        else if(dialog != QMessageBox::Discard)
            return;
    }

    if(!openFilename.length())
        openFilename = QFileDialog::getOpenFileName(this, "Open Sheet", lastImportExportStr, "Sprite Sheets (*.sheet)");

    if(openFilename.length())
    {
        QFile f(openFilename);
        if(f.open(QIODevice::ReadOnly))
        {
            lastImportExportStr = openFilename;
            mRecentDocuments->addDocument(openFilename);

            QDataStream s(&f);
            s.setVersion(QDataStream::Qt_5_4);

            //Clean up memory
            cleanMemory();

            loadFromStream(s);

            QFileInfo fi(openFilename);
            sCurFilename = fi.fileName();
            bFileModified = false;
            fixWindowTitle();
            lastSaveStr = openFilename;
            //TODO Store file orig state
            clearUndo();
            clearRedo();
            pushUndo();
            updateUndoRedoMenu();
        }
    }
}

void MainWindow::saveToStream(QDataStream& s)
{
    //Save sheet frames
    int curAnim = 0, curFrame = 0;
    int cnt = 0;
    int major = MAJOR_VERSION;
    int minor = MINOR_VERSION;
    int rev = REV_VERSION;
    s << major << minor << rev;  //Later we'll care about this if the save format changes again
    s << mSheetFrames.size();
    for(QList<QList<QImage> >::iterator i = mSheetFrames.begin(); i != mSheetFrames.end(); i++)
    {
        if(i == mCurAnim)
            curAnim = cnt;
        cnt++;

        s << i->size();
        int innercnt = 0;
        for(QList<QImage>::iterator j = i->begin(); j != i->end(); j++)
        {
            if(i == mCurAnim && j == mCurFrame)
                curFrame = innercnt;
            innercnt++;

            QByteArray imgByteArray;
            QBuffer buffer(&imgByteArray);
            buffer.open(QIODevice::WriteOnly);
            j->save(&buffer, "TIFF");
            s << imgByteArray;
        }
    }

    //Save anim names
    s << mAnimNames.size();
    foreach(QString str, mAnimNames)
        s << str;

    //Save other stuff
    s << sheetBgCol;
    s << frameBgCol;
    s << ui->FrameBgTransparent->isChecked() << ui->SheetBgTransparent->isChecked();
    s << ui->xSpacingBox->value() << ui->ySpacingBox->value() << ui->sheetWidthBox->value();
    s << sheetFont.toString();
    s << curAnim << curFrame;
    s << ui->animNameEnabled->isChecked();
}

void MainWindow::loadFromStream(QDataStream& s)
{
    bLoadMutex = true;
    //Grab sheet frames
    int major = MAJOR_VERSION;
    int minor = MINOR_VERSION;
    int rev = REV_VERSION;
    s >> major >> minor >> rev;  //Later we'll care about this if the save format changes again
    int numAnims = 0;
    s >> numAnims;
    for(int i = 0; i < numAnims; i++)
    {
        QList<QImage> imgList;
        int numFrames = 0;
        s >> numFrames;
        for(int j = 0; j < numFrames; j++)
        {
            //Default image saving (PNG) is sloooowwww... save as TIFF instead. Larger but way faster.
            QImage img;
            QByteArray imgByteArray;
            s >> imgByteArray;
            QBuffer buffer(&imgByteArray);
            buffer.open(QIODevice::ReadOnly);
            img.load(&buffer, "TIFF");
            imgList.push_back(img);
        }
        mSheetFrames.push_back(imgList);
    }

    //Grab anim names
    int numAnimNames = 0;
    s >> numAnimNames;
    for(int i = 0; i < numAnimNames; i++)
    {
        QString str;
        s >> str;
        mAnimNames.push_back(str);
    }

    //Read other stuff
    s >> sheetBgCol;
    s >> frameBgCol;

    //Fill in frame/sheet colors
    QPixmap colIcon(32, 32);
    colIcon.fill(sheetBgCol);
    QIcon ic(colIcon);
    ui->sheetBgColSelect->setIcon(ic);
    colIcon.fill(frameBgCol);
    ic = QIcon(colIcon);
    ui->frameBgColSelect->setIcon(ic);

    bool bSheetBg, bFrameBg;
    s >> bFrameBg >> bSheetBg;
    ui->FrameBgTransparent->setChecked(bFrameBg);
    ui->SheetBgTransparent->setChecked(bSheetBg);
    ui->frameBgColSelect->setEnabled(!bFrameBg);
    ui->sheetBgColSelect->setEnabled(!bSheetBg);
    int xSpacing, ySpacing, sheetWidth;
    s >> xSpacing >> ySpacing >> sheetWidth;
    ui->xSpacingBox->setValue(xSpacing);
    ui->ySpacingBox->setValue(ySpacing);
    ui->sheetWidthBox->setValue(sheetWidth);
    QString sFontStr;
    s >> sFontStr;
    if(s.status() == QDataStream::Ok)
        sheetFont.fromString(sFontStr);
    int curAnim = 0, curAnimFrame = 0;  //Read current anim/anim frame
    s >> curAnim >> curAnimFrame;
    if(s.status() != QDataStream::Ok)
        curAnim = curAnimFrame = 0;
    bool bNamesEnabled;
    s >> bNamesEnabled;
    if(s.status() == QDataStream::Ok)
        ui->animNameEnabled->setChecked(bNamesEnabled);

    //Done reading


    //Default to beginning of animation and frame lists...
    mCurAnim = mSheetFrames.begin();
    mCurAnimName = mAnimNames.begin();
    if(mCurAnim != mSheetFrames.end())
        mCurFrame = mCurAnim->begin();

    //Set to correct anim and frame...
    int cnt = 0;
    for(QList<QList<QImage> >::iterator i = mSheetFrames.begin(); i != mSheetFrames.end(); i++, cnt++)
    {
        if(cnt == curAnim)
        {
            mCurFrame = i->begin();
            mCurAnim = i;
            int innercnt = 0;
            for(QList<QImage>::iterator j = i->begin(); j != i->end(); j++, innercnt++)
            {
                if(innercnt == curAnimFrame)
                {
                    mCurFrame = j;
                    break;
                }
            }
            break;
        }
    }

    //Set to correct anim name...
    cnt = 0;
    for(QList<QString>::iterator i = mAnimNames.begin(); i != mAnimNames.end(); i++, cnt++)
    {
        if(cnt == curAnim)
        {
            mCurAnimName = i;
            break;
        }
    }

    //Reset GUI stuff!
    drawSheet();
    if(mCurAnimName != mAnimNames.end())
        ui->animationNameEditor->setText(*mCurAnimName);

    drawAnimation();
    bLoadMutex = false;
}

void MainWindow::cleanMemory()
{
    if(mCurSheet)
        delete mCurSheet;
    mCurSheet = NULL;

    mSheetFrames.clear();
    mCurAnim = mSheetFrames.begin();
    mAnimNames.clear();
    mCurAnimName = mAnimNames.begin();
}

void MainWindow::on_fontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, sheetFont, this);
    if(ok)
    {
        sheetFont = font;
        drawSheet();
        genUndoState();
    }
}

void MainWindow::fixWindowTitle()
{
    QString sWindowStr;
    QTextStream sWindowTitle(&sWindowStr);
    if(bFileModified)
        sWindowTitle << "*";
    sWindowTitle << sCurFilename;
    sWindowTitle << " - Sprite Sheeter v" << MAJOR_VERSION << "." << MINOR_VERSION;
    if(REV_VERSION)
        sWindowTitle << " r" << REV_VERSION;
    setWindowTitle(sWindowStr);
}

void MainWindow::genUndoState()
{
    if(mSheetFrames.size() == 0 && sCurFilename == UNTITLED_IMAGE_STR && undoList.size() < 2)
    {
        clearUndo();
        pushUndo(); //Save this as our starting state
        return;   //Don't generate undo states on empty sheet
    }

    //Set the window title if this is the first the file has been modified
    if(!bFileModified)
    {
        bFileModified = true;
        fixWindowTitle();
    }

    //Gen undo point
    pushUndo();

    //Clear redo list
    clearRedo();

    updateUndoRedoMenu();
}

void MainWindow::pushUndo()
{
    QByteArray* baUndoPt = new QByteArray();
    baUndoPt->reserve(2600000); //Reserve some space here so we aren't reallocating over and over
    QDataStream s(baUndoPt, QIODevice::WriteOnly);
    saveToStream(s);
    undoList.push(baUndoPt);
}

void MainWindow::clearUndo()
{
    while(undoList.size())
    {
        QByteArray* ba = undoList.pop();
        delete ba;
    }
}

void MainWindow::clearRedo()
{
    while(redoList.size())
    {
        QByteArray* ba = redoList.pop();
        delete ba;
    }
}

void MainWindow::updateUndoRedoMenu()
{
    ui->actionRedo->setEnabled(redoList.size() > 0);
    ui->actionUndo->setEnabled(undoList.size() > 1);
}

//TODO Only gen undo states here if different
void MainWindow::on_xSpacingBox_editingFinished()
{
    drawSheet();
    genUndoState();
}

void MainWindow::on_ySpacingBox_editingFinished()
{
    drawSheet();
    genUndoState();
}

void MainWindow::on_sheetWidthBox_editingFinished()
{
    drawSheet();
    genUndoState();
}

void MainWindow::on_animationNameEditor_editingFinished()
{
    drawSheet();
    genUndoState();
}

void MainWindow::enableShortcuts(bool b)
{
    bShortcuts = b;
}

void MainWindow::on_animNameEnabled_toggled(bool checked)
{
    if(checked)
    {
        ui->animationNameEditor->setEnabled(true);
        ui->fontButton->setEnabled(true);
    }
    else
    {
        ui->animationNameEditor->setEnabled(false);
        ui->fontButton->setEnabled(false);
    }
    drawSheet();
    genUndoState();
}

FIBITMAP* imageFromPixels(uint8_t* imgData, uint32_t width, uint32_t height)
{
    FIBITMAP* curImg = FreeImage_Allocate(width, height, 32);
    FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(curImg);
    if(image_type == FIT_BITMAP)
    {
        int curPos = 0;
        unsigned pitch = FreeImage_GetPitch(curImg);
        BYTE* bits = (BYTE*)FreeImage_GetBits(curImg);
        bits += pitch * height - pitch;
        for(int y = height-1; y >= 0; y--)
        {
            BYTE* pixel = (BYTE*)bits;
            for(uint32_t x = 0; x < width; x++)
            {
                pixel[FI_RGBA_BLUE] = imgData[curPos++];
                pixel[FI_RGBA_GREEN] = imgData[curPos++];
                pixel[FI_RGBA_RED] = imgData[curPos++];
                pixel[FI_RGBA_ALPHA] = imgData[curPos++];
                //Quantize low-alpha to magenta by hand...
                if(pixel[FI_RGBA_ALPHA] <= 128)
                {
                    pixel[FI_RGBA_RED] = 255;
                    pixel[FI_RGBA_GREEN] = 0;
                    pixel[FI_RGBA_BLUE] = 255;
                    pixel[FI_RGBA_ALPHA] = 255;
                }
                pixel += 4;
            }
            bits -= pitch;
        }
    }
    return curImg;
}

//(see http://sourceforge.net/p/freeimage/discussion/36111/thread/ea987d97/) for discussion of FreeImage gif saving...
void MainWindow::on_ExportAnimButton_clicked()
{
    if(!mCurSheet || mSheetFrames.empty() || mCurAnim == mSheetFrames.end()) return;

    QString saveFilename = QFileDialog::getSaveFileName(this,
                                                        tr("Save GIF Animation"),
                                                        lastGIFStr,
                                                        tr("GIF Image (*.gif)"));

    if(saveFilename.length())
    {
        lastGIFStr = saveFilename;

        //Open GIF image for writing
        FIMULTIBITMAP* bmp = FreeImage_OpenMultiBitmap(FIF_GIF, saveFilename.toStdString().c_str(), true, false);
        DWORD dwFrameTime = (DWORD)((1000.0f / (float)ui->animationSpeedSpinbox->value()) + 0.5f); //Framerate of gif image

        for(QList<QImage>::iterator i = mCurAnim->begin(); i != mCurAnim->end(); i++)
        {
            //Create image and 256-color image
            QImage imgTemp(*i);
            //Gotta get Qt image in proper format first
            imgTemp = imgTemp.convertToFormat(QImage::Format_ARGB32);
            QByteArray bytes((char*)imgTemp.bits(), imgTemp.byteCount());
            //Make 32-bit image with magenta instead of transparency first...
            FIBITMAP* page = imageFromPixels((uint8_t*)bytes.data(), imgTemp.width(), imgTemp.height());
            //Turn this into an 8-bit image next
            FIBITMAP* page8bit = FreeImage_ColorQuantize(page, FIQ_WUQUANT);

            //Set transparency table from magenta. *Hopefully this was preserved during quantization!*
            RGBQUAD *Palette = FreeImage_GetPalette(page8bit);
            BYTE Transparency[256];
            for (unsigned i = 0; i < 256; i++)
            {
                Transparency[i] = 0xFF;
                if(Palette[i].rgbGreen == 0x00 &&
                   Palette[i].rgbBlue == 0xFF &&
                   Palette[i].rgbRed == 0xFF)
                {
                    Transparency[i] = 0x00;
                }
            }
            FreeImage_SetTransparencyTable(page8bit, Transparency, 256);

            //Append metadata - frame speed based on current playback speed
            FreeImage_SetMetadata(FIMD_ANIMATION, page8bit, NULL, NULL);
            FITAG *tag = FreeImage_CreateTag();
            if(tag)
            {
                FreeImage_SetTagKey(tag, "FrameTime");
                FreeImage_SetTagType(tag, FIDT_LONG);
                FreeImage_SetTagCount(tag, 1);
                FreeImage_SetTagLength(tag, 4);
                FreeImage_SetTagValue(tag, &dwFrameTime);
                FreeImage_SetMetadata(FIMD_ANIMATION, page8bit, FreeImage_GetTagKey(tag), tag);
                FreeImage_DeleteTag(tag);
            }
            FreeImage_AppendPage(bmp, page8bit);
            FreeImage_Unload(page);
            FreeImage_Unload(page8bit);
        }

        //Save final GIF
        FreeImage_CloseMultiBitmap(bmp, GIF_DEFAULT);
    }
}

bool MainWindow::loadAnimatedGIF(QString sFilename)
{
    FIMULTIBITMAP* bmp = FreeImage_OpenMultiBitmap(FIF_GIF, sFilename.toStdString().c_str(), false, true);

    if(bmp == NULL) return false;

    int numFrames = FreeImage_GetPageCount(bmp);

    if(numFrames < 2)   //If only one frame in this image, pass it along as a multi-frame single-image bitmap
    {
        FreeImage_CloseMultiBitmap(bmp);
        return false;
    }

    //Grab all the frames and stick them in a Qt-friendly format
    QList<QImage> frameList;
    for(int i = 0; i < numFrames; i++)
    {
        FIBITMAP* frame = FreeImage_LockPage(bmp, i);
        FIBITMAP* frame32bit = FreeImage_ConvertTo32Bits(frame);

        QImage imgResult(FreeImage_GetBits(frame32bit), FreeImage_GetWidth(frame32bit), FreeImage_GetHeight(frame32bit), FreeImage_GetPitch(frame32bit), QImage::Format_ARGB32);
        frameList.push_back(imgResult.mirrored());

        FreeImage_Unload(frame32bit);   //Qt expects the memory to be available the whole time wut? Luckily, we have to mirror it anyway so it doesn't matter
        FreeImage_UnlockPage(bmp, frame, false);
    }

    QString fileName = QFileInfo(sFilename).baseName();
    insertAnimHelper(frameList, fileName);

    if(mCurAnim != mSheetFrames.end())
        mCurFrame = mCurAnim->begin();

    drawSheet();
    drawAnimation();
    genUndoState();

    FreeImage_CloseMultiBitmap(bmp);
    return true;
}

void MainWindow::on_reverseAnimButton_clicked()
{
    if(mCurAnim == mSheetFrames.end()) return;
    if(mCurAnim->size() < 2) return;
    QList<QImage> newList = *mCurAnim;
    mCurAnim->clear();
    foreach(QImage img, newList)
        mCurAnim->prepend(img);


    mCurFrame = mCurAnim->begin();
    drawSheet();
    drawAnimation();
    genUndoState();
}

void MainWindow::on_removeDuplicateFramesButton_clicked()
{
    if(mCurAnim == mSheetFrames.end()) return;
    if(mCurAnim->size() < 2) return;
    bool bFoundDuplicates = false;

    for(int tester = 0; tester < mCurAnim->size(); tester++)
    {
        for(int testee = tester+1; testee < mCurAnim->size(); testee++)
        {
            if((*mCurAnim)[testee].width() != (*mCurAnim)[tester].width() || (*mCurAnim)[testee].height() != (*mCurAnim)[tester].height()) continue;
            if((*mCurAnim)[testee].byteCount() != (*mCurAnim)[tester].byteCount()) continue;

            if(std::strncmp((const char*)(*mCurAnim)[testee].bits(), (const char*)(*mCurAnim)[tester].bits(), (*mCurAnim)[testee].byteCount()) == 0)
            {
                bFoundDuplicates = true;
                mCurAnim->removeAt(testee);
                testee--;
            }
        }
    }

    if(bFoundDuplicates)
    {
        mCurFrame = mCurAnim->begin();
        drawSheet();
        drawAnimation();
        genUndoState();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QString nameVersion = "Sprite Sheeter v" + QString::number(MAJOR_VERSION) + "." + QString::number(MINOR_VERSION);
    if(REV_VERSION)
        nameVersion += " r" + QString::number(REV_VERSION);
    QString aboutText = "<h3>" + nameVersion + "</h3>" +
            "<p>Author: <a href=\"http://www.vg-resource.com/user-23255.html\">Daxar</a></p>" +
            "<p>Questions/comments? Ping me on the Spriters Resource forum!</p>";
    QMessageBox::about(this, "About Sprite Sheeter", aboutText);
}

void MainWindow::on_actionBatch_Processing_triggered()
{
    //HACK The standard Windows dialogs don't allow selection of multiple folders; make one manually
    QFileDialog* multiSelectFolder = new QFileDialog(this, "Select folders for batch processing");
    multiSelectFolder->setFileMode(QFileDialog::DirectoryOnly);
    multiSelectFolder->setOption(QFileDialog::DontUseNativeDialog, true);
    multiSelectFolder->setDirectory(lastOpenDir);
    QListView *listView = multiSelectFolder->findChild<QListView*>("listView");
    if(listView)
        listView->setSelectionMode(QAbstractItemView::MultiSelection);
    QTreeView *treeView = multiSelectFolder->findChild<QTreeView*>();
    if(treeView)
        treeView->setSelectionMode(QAbstractItemView::MultiSelection);
    multiSelectFolder->exec();
    QStringList fileNames = multiSelectFolder->selectedFiles();
    delete multiSelectFolder;

    //Spin off threads to render these
    for(QStringList::const_iterator it = fileNames.begin(); it != fileNames.end(); it++)
    {
        //qDebug() << *it;

        BatchRenderer* batchRenderer = new BatchRenderer();
        batchRenderer->folder = *it;
        batchRenderer->sheetFont = sheetFont;
        batchRenderer->maxSheetWidth = ui->sheetWidthBox->value();
        batchRenderer->offsetX = ui->xSpacingBox->value();
        batchRenderer->offsetY = ui->ySpacingBox->value();
        batchRenderer->animNameEnabled = ui->animNameEnabled->isChecked();
        batchRenderer->sheetBgTransparent = ui->SheetBgTransparent->isChecked();
        batchRenderer->sheetBgCol = sheetBgCol;
        batchRenderer->animHighlightCol = animHighlightCol;
        batchRenderer->frameBgTransparent = ui->FrameBgTransparent->isChecked();
        batchRenderer->frameBgCol = frameBgCol;

        //TODO Dialog with % complete to hook up to this
        //QObject::connect(batchRenderer, SIGNAL(renderingStart(QString)), this, SLOT(startedBatchRender(QString)));
        //QObject::connect(batchRenderer, SIGNAL(renderingDone(QString)), this, SLOT(finishedBatchRender(QString)));

        QThreadPool::globalInstance()->start(batchRenderer);
    }

}










































