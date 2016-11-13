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

#define SELECT_RECT_THICKNESS 5

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    sheet = NULL;

    //Create UI, removing help icons as needed
    ui->setupUi(this);
    mImportWindow = new ImportDialog(this);
    mImportWindow->setWindowFlags(mImportWindow->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    mBalanceWindow = new BalanceSheetDialog(this);
    mBalanceWindow->setWindowFlags(mBalanceWindow->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    mIconExportWindow = new IconExportDialog(this);
    mIconExportWindow->setWindowFlags(mIconExportWindow->windowFlags() & ~Qt::WindowContextHelpButtonHint);
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
    QObject::connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(saveFileAs()));
    QObject::connect(ui->actionImport_WIP_Sheet, SIGNAL(triggered(bool)), this, SLOT(loadSheet()));
    QObject::connect(ui->actionUndo, SIGNAL(triggered(bool)), this, SLOT(undo()));
    QObject::connect(ui->actionRedo, SIGNAL(triggered(bool)), this, SLOT(redo()));
    QObject::connect(ui->sheetPreview, SIGNAL(droppedFiles(QStringList)), this, SLOT(addImages(QStringList)));
    QObject::connect(ui->sheetPreview, SIGNAL(droppedFolders(QStringList)), this, SLOT(addFolders(QStringList)));
    QObject::connect(mBalanceWindow, SIGNAL(balance(int,int,BalancePos::Pos,BalancePos::Pos)), this, SLOT(balance(int,int,BalancePos::Pos,BalancePos::Pos)));
    QObject::connect(this, SIGNAL(setBalanceDefWH(int,int)), mBalanceWindow, SLOT(defaultWH(int,int)));
    QObject::connect(this, SIGNAL(setIconImage(QImage)), mIconExportWindow, SLOT(setImage(QImage)));
    QObject::connect(mRecentDocuments, SIGNAL(openFile(QString)), this, SLOT(loadSheet(QString)));

    animItem = NULL;
    progressBar = NULL;
    transparentBg = new QImage("://bg");
    bLoadMutex = false;

    bDraggingSheetW = false;
    m_bDraggingSelected = false;
    m_bSetDraggingCursor = false;
    bFileModified = false;
    m_rLastDragHighlight.setCoords(0,0,0,0);
    m_bLastDragInAnim = false;
    sCurFilename = UNTITLED_IMAGE_STR;
    //Store initial undo state
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

    mSheetZoom = new ZoomableGraphicsView(ui->sheetPreview);
    mAnimationZoom = new ZoomableGraphicsView(ui->animationPreview);

    sheetBgCol = QColor(0, 128, 128, 255);
    frameBgCol = QColor(0, 255, 0, 255);
    animHighlightCol = QColor(128, 0, 0, 255);
    fontColor = QColor(255, 255, 255);

    //Create animation sheet
    sheet = new Sheet(msheetScene, ui->sheetPreview, transparentBg, DRAG_HANDLE_SIZE);

    if(ui->sheetPreview->isHidden())
        ui->sheetPreview->show();

    //Read in settings here
    loadSettings();

    //Set color icons to proper color
    setColorButtonIcons();
    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    clearUndo();
    clearRedo();
    if(transparentBg)
        delete transparentBg;
    if(animItem)
        delete animItem;
    delete sheet;
    if(animScene)
        delete animScene;
    if(msheetScene)
        delete msheetScene;
    delete mIconExportWindow;
    delete animUpdateTimer;
    delete mImportWindow;
    delete mBalanceWindow;
    delete mSheetZoom;
    delete mAnimationZoom;
    delete ui;
}

void MainWindow::addImages(QStringList l)
{
    mOpenFiles = l;
    openImportDiag();
}

void MainWindow::importImageList(QStringList& fileList, QString prepend, QString animName)
{
    //TODO set anim name
    Q_UNUSED(animName)

    if(fileList.size())
    {
        Animation* animation = new Animation(transparentBg, this);
        foreach(QString s1, fileList)
        {
            QString imgPath = prepend + s1;
            QImage* image = new QImage(imgPath);
            if(!image->isNull())
                animation->insertImage(image, msheetScene);
            else
                qDebug() << "Unable to open image " << imgPath << endl;
        }
        sheet->addAnimation(animation);

        minimizeSheetWidth();
        drawAnimation();
        genUndoState();
    }
}

QStringList MainWindow::supportedFileFormats()
{
    QStringList fileFilters;
    //TODO: Rather than specifying manually, remove filter entirely or add ALL actual file types we support
    fileFilters << "*.png"
                << "*.dds"
                << "*.bmp"
                << "*.gif"
                << "*.pbm"
                << "*.pgm"
                << "*.ppm"
                << "*.tif"
                << "*.tiff"
                << "*.xbm"
                << "*.xpm"
                << "*.tga"
                << "*.jpg"
                << "*.jpeg";
    return fileFilters;
}

void MainWindow::addFolders(QStringList l)
{
    foreach(QString s, l)
    {
        QDir folder(s);
        //Filter out only image files
        QStringList fileFilters = supportedFileFormats();
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
    importImageAsSheet(curImportImage, numx, numy, bVert, bSplit);
    openImportDiag();   //Next one
}

void MainWindow::importAll(int numx, int numy, bool bVert, bool bSplit)
{
    importImageAsSheet(curImportImage, numx, numy, bVert, bSplit);
    foreach(QString s, mOpenFiles)
        importImageAsSheet(s, numx, numy, bVert, bSplit);

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
        centerParent(this, mImportWindow);
    }
}

void MainWindow::insertAnimHelper(QVector<QImage*> imgList, QString name)
{
    //TODO Create label
    Q_UNUSED(name)

    if(imgList.size())
    {
        //TODO Insert above (below?) current animation
        Animation* animation = new Animation(transparentBg, this);
        animation->insertImages(imgList, msheetScene);
        sheet->addAnimation(animation);
    }
}

void MainWindow::importImageAsSheet(QString s, int numxframes, int numyframes, bool bVert, bool bSplit)
{
    QImage image(s);
    if(image.isNull())
    {
        QMessageBox::information(this, "Image Import", "Error opening image " + s);
        return;
    }
    QString fileName = QFileInfo(s).baseName();

    //Find image dimensions
    int iXFrameSize = image.width() / numxframes;
    int iYFrameSize = image.height() / numyframes;

    //Grab all the frames out
    QVector<QImage*> imgList;
    if(!bVert)
    {
        for(int y = 0; y < numyframes; y++)
        {
            for(int x = 0; x < numxframes; x++)
            {
                imgList.push_back(new QImage(image.copy(x*iXFrameSize, y*iYFrameSize, iXFrameSize, iYFrameSize)));
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
                imgList.push_back(new QImage(image.copy(x*iXFrameSize, y*iYFrameSize, iXFrameSize, iYFrameSize)));
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

    drawAnimation();
    genUndoState();
}

//Example from http://www.qtforum.org/article/28852/center-any-child-window-center-parent.html
void MainWindow::centerParent(QWidget* parent, QWidget* child)
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

void MainWindow::on_xSpacingBox_valueChanged(int arg1)
{
    if(sheet)
        sheet->setXSpacing(arg1);
}

void MainWindow::on_ySpacingBox_valueChanged(int arg1)
{
    if(sheet)
        sheet->setYSpacing(arg1);
}

void MainWindow::genericSave(QString saveFilename)
{
    if(saveFilename.length())
    {
        //drawSheet(false);   //Save a non-highlighted version
        //TODO Create image and save
        lastSaveStr = saveFilename;
        if(saveFilename.contains(".sheet", Qt::CaseInsensitive))
        {
            saveSheet(saveFilename);
            QFileInfo fi(saveFilename);
            sCurFilename = fi.fileName();
            bFileModified = false;
            updateWindowTitle();
            //TODO Store file orig state
        }
        else
        {
            //if(!mCurSheet->save(saveFilename))
            {
                //TODO
                QMessageBox::information(this,"Image Export","Error saving image " + saveFilename);
            }
            //            else
            //            {
            //                QFileInfo fi(saveFilename);
            //                sCurFilename = fi.fileName();
            //                bFileModified = false;
            //                fixWindowTitle();
            //                //TODO Store file orig state
            //            }
        }
        //drawSheet(true);    //Redraw the highlighted version
    }
}

QString MainWindow::getSaveFilename(const char* title)
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

    return QFileDialog::getSaveFileName(this,
                                        tr(title),
                                        lastSaveStr,
                                        tr("PNG Image (*.png);;Windows Bitmap (*.bmp);;TIFF Image (*.tiff);;Sprite Sheet (*.sheet)"),
                                        &sSel);
}

//Save file
void MainWindow::saveFile()
{
    if(!sheet || !sheet->size())
        return;

    if(!bFileModified)
        return;  //Don't bother saving if we already have

    QString saveFilename;
    if(sCurFilename == UNTITLED_IMAGE_STR)  //Haven't saved this yet
        saveFilename = getSaveFilename("Save Sheet");
    else
        saveFilename = lastSaveStr;

    genericSave(saveFilename);
}

void MainWindow::saveFileAs()
{
    if(!sheet || !sheet->size())
        return;

    genericSave(getSaveFilename("Save Sheet As"));
}

void MainWindow::on_removeAnimButton_clicked()
{
    //TODO Wipe current animation from sheet

    drawAnimation();
    genUndoState();
}

void MainWindow::on_animationNameEditor_textChanged(const QString& arg1)
{
    Q_UNUSED(arg1)
    //TODO Update current label
}

void MainWindow::drawAnimation()
{
    //TODO

    //    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size() || mCurFrame == mCurAnim->end())
    //    {
    //        if(animItem)
    //            animItem->hide();
    //        return;
    //    }

    //Draw image and bg
    //    QImage animFrame(mCurFrame->width(), mCurFrame->height(), QImage::Format_ARGB32);
    //    QPainter painter(&animFrame);
    //    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    //    if(ui->FrameBgTransparent->isChecked())
    //    {
    //        QBrush bgTexBrush(*transparentBg);
    //        painter.fillRect(0, 0, mCurFrame->width(), mCurFrame->height(), bgTexBrush);
    //    }
    //    else
    //        animFrame.fill(frameBgCol);

    //    painter.drawImage(0, 0, *mCurFrame);


    //    if(animItem == NULL)
    //    {
    //        animItem = new QGraphicsPixmapItem(QPixmap::fromImage(animFrame));
    //        animScene->addItem(animItem);
    //    }
    //    else
    //        animItem->setPixmap(QPixmap::fromImage(animFrame));

    //    animItem->show();
    //    animScene->setSceneRect(0, 0, animFrame.width(), animFrame.height());
}

void MainWindow::animUpdate()
{
    //TODO

    //    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
    //        return;

    //    if(mCurFrame == mCurAnim->end())
    //        mCurFrame = mCurAnim->begin();
    //    else
    //    {
    //        mCurFrame++;
    //        if(mCurFrame == mCurAnim->end())
    //            mCurFrame = mCurAnim->begin();
    //    }

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
    //TODO Update icon
    if(!animUpdateTimer->isActive())
    {
        int iInterval = 1000/ui->animationSpeedSpinbox->value();
        animUpdateTimer->start(iInterval);
    }
    else
    {
        animUpdateTimer->stop();
    }

}

void MainWindow::on_animStopButton_clicked()
{
    animUpdateTimer->stop();
    //TODO Set current animation frame

    //    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
    //        return;

    //    mCurFrame = mCurAnim->begin();

    drawAnimation();
}

void MainWindow::on_animPrevFrameButton_clicked()
{
    if(animUpdateTimer->isActive())
        animUpdateTimer->stop();
    //TODO Set current animation frame

    //    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
    //        return;

    //    if(mCurFrame == mCurAnim->begin())
    //        mCurFrame = mCurAnim->end();
    //    mCurFrame--;

    drawAnimation();
}

void MainWindow::on_animNextFrameButton_clicked()
{
    if(animUpdateTimer->isActive())
        animUpdateTimer->stop();
    //TODO Set current animation frame

    //    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size())
    //        return;

    //    mCurFrame++;
    //    if(mCurFrame == mCurAnim->end())
    //        mCurFrame = mCurAnim->begin();

    drawAnimation();
}

bool MainWindow::isMouseOverDragArea(int x, int y)
{
    if(x < 0 || y < 0)
        return false;

    unsigned int ux = x;
    unsigned int uy = y;
    return ux >= sheet->getWidth() &&
            ux <= sheet->getWidth() + DRAG_HANDLE_SIZE &&
            uy <= sheet->getHeight() &&
            uy >= 0;
}

QGraphicsItem* MainWindow::isItemUnderCursor(int x, int y)
{
    //Get the list of all items under this position
    QList<QGraphicsItem *> items = msheetScene->items(
                x,
                y,
                1,
                1,
                Qt::IntersectsItemBoundingRect, //Intersect the bounding rectangle of the item (because transparency)
                Qt::AscendingOrder,             //No idea if this is what we want
                mSheetZoom->getView()->transform());

    //Grab the topmost animation frame image
    foreach(QGraphicsItem* item, items)
    {
        if(!item->zValue()) //our animation frames are at a z of 0
            return item;
    }
    return NULL;
}

void MainWindow::mouseCursorPos(int x, int y)
{
    if(sheet)
    {
        //Update cursor if need be
        if(isMouseOverDragArea(x, y))
            ui->sheetPreview->setCursor(Qt::SizeHorCursor);

        else if(!bDraggingSheetW && !m_bDraggingSelected)
            ui->sheetPreview->setCursor(Qt::ArrowCursor);

        //If dragging, update sheet width (set the box value directly so that it updates properly)
        if(bDraggingSheetW)
        {
            ui->sheetWidthBox->setValue(mStartSheetW - (xStartDragSheetW - x));
            if(ui->minWidthCheckbox->isChecked())
                minimizeSheetWidth();
        }
    }

    static QGraphicsRectItem* curSelectedRect = NULL;
    QGraphicsItem* it = isItemUnderCursor(x, y);
    if(it != NULL)
    {
        //Draw box around currently-highlighted image
        if(curSelectedRect == NULL)
            curSelectedRect = msheetScene->addRect(it->boundingRect(), QPen(QColor(0,0,255), SELECT_RECT_THICKNESS));
        curSelectedRect->setRect(it->boundingRect());
        curSelectedRect->setPos(it->x(), it->y());
        curSelectedRect->setVisible(true);
        curSelectedRect->setZValue(2); //Above everything
    }
    else
    {
        if(curSelectedRect != NULL)
            curSelectedRect->setVisible(false);
    }

    //Show the mouse cursor pos
    statusBar()->showMessage(QString::number(x) + ", " + QString::number(y));
    curMouseY = y;
    curMouseX = x;
}

void MainWindow::mouseDown(int x, int y)
{
    if(sheet)
    {
        //We're starting to drag the sheet size handle
        if(isMouseOverDragArea(x, y))
        {
            bDraggingSheetW = true;
            mStartSheetW = sheet->getWidth();
            xStartDragSheetW = x;
        }

        //TODO

        //        else
        //        {
        //            //Figure out what anim we're clicking on
        //            if(!mAnimRects.empty() && !mSheetFrames.empty())
        //            {
        //                QList<QList<QImage> >::iterator i = mSheetFrames.begin();
        //                QList<QString>::iterator name = mAnimNames.begin();
        //                foreach(QRect r, mAnimRects)
        //                {
        //                    if(r.contains(x,y))
        //                    {
        //                        mCurAnim = i;
        //                        mCurAnimName = name;

        //                        drawSheet();
        //                        if(mCurAnimName != mAnimNames.end())
        //                            ui->animationNameEditor->setText(*mCurAnimName);
        //                        else
        //                            ui->animationNameEditor->setText(QString(""));

        //                        if(mCurAnim != mSheetFrames.end())
        //                            mCurFrame = mCurAnim->begin();
        //                        drawAnimation();

        //                        break;
        //                    }
        //                    i++;
        //                    name++;
        //                }
        //            }

        //            //Start dragging if we should...
        //            if(mCurSelectedInAnim != mSheetFrames.end())
        //            {
        //                m_bDraggingSelected = true;
        //                m_bSetDraggingCursor = false;
        //                m_selDragToAnim = mSheetFrames.end();
        //                m_rLastDragHighlight.setCoords(0,0,0,0);
        //            }
        //        }
    }
}

void MainWindow::mouseUp(int x, int y)
{
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(sheet)
    {
        if(bDraggingSheetW)
        {
            bDraggingSheetW = false;
            sheet->updateSceneBounds();
            //genUndoState();
        }
    }

    //TODO

    //        if(m_bDraggingSelected)
    //        {
    //            m_bDraggingSelected = false;
    //            if(!mAnimRects.empty() && !mSheetFrames.empty())
    //            {
    //                QList<QList<QImage> >::iterator i = mSheetFrames.begin();
    //                QList<QString>::iterator name = mAnimNames.begin();
    //                bool bDropped = false;
    //                foreach(QRect r, mAnimRects)
    //                {
    //                    if(y >= r.top() && y <= r.bottom())//r.contains(x,y))
    //                    {
    //                        //Drop off in position...
    //                        if(m_selDragToAnim != mSheetFrames.end())
    //                        {
    //                            //If we're dragging within an animation, just swap positions
    //                            if(m_selDragToAnim == mCurSelectedInAnim)
    //                            {
    //                                int startpos = mCurSelected - mCurSelectedInAnim->begin();
    //                                int endpos = m_selDragToPos - m_selDragToAnim->begin();
    //                                if(startpos != endpos && startpos != endpos-1)  //Make sure we're not dragging into itself
    //                                {
    //                                    if(endpos < startpos)
    //                                        endpos++;
    //                                    if(endpos <= 0)
    //                                        m_selDragToAnim->move(startpos, 0);
    //                                    else
    //                                        m_selDragToAnim->move(startpos, endpos-1);
    //                                }
    //                                mCurFrame = mCurAnim->begin();
    //                            }
    //                            else    //Move to another anim
    //                            {
    //                                QImage img = *mCurSelected;
    //                                m_selDragToAnim->insert(m_selDragToPos, img);
    //                                mCurSelectedInAnim->erase(mCurSelected);
    //                                mCurAnim = mCurSelectedInAnim;
    //                                mCurFrame = mCurAnim->begin();
    //                                if(mCurFrame == mCurAnim->end())    //We erased this whole animation; clean up
    //                                    on_removeAnimButton_clicked();  //Simulate deleting this anim
    //                            }
    //                            genUndoState();
    //                        }

    //                        bDropped = true;
    //                        break;
    //                    }
    //                    i++;
    //                    name++;
    //                }

    //                if(!bDropped)
    //                {
    //                    if(y > mCurSheet->height())
    //                    {
    //                        //Create new anim for this frame
    //                        QImage img = *mCurSelected;
    //                        mCurSelectedInAnim->erase(mCurSelected);
    //                        mCurFrame = mCurAnim->begin();
    //                        if(mCurFrame == mCurAnim->end())
    //                        {
    //                            //We erased this whole animation; clean up
    //                            on_removeAnimButton_clicked();  //Simulate deleting this anim
    //                        }
    //                        QList<QImage> qNewAnim;
    //                        qNewAnim.push_back(img);
    //                        mSheetFrames.push_back(qNewAnim);
    //                        mAnimNames.push_back("");
    //                        mCurAnimName = mAnimNames.end();
    //                        mCurAnimName--;
    //                        mCurAnim = mSheetFrames.end();
    //                        mCurAnim--;
    //                        mCurFrame = mCurAnim->begin();
    //                        genUndoState();
    //                    }
    //                }
    //            }
    //            drawSheet();
    //            drawAnimation();
    //            mouseCursorPos(x,y);
    //        }
    //    }
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
            saveFile();
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
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::saveSettings()
{
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
    settings.setValue("fontColr", fontColor.red());
    settings.setValue("fontColg", fontColor.green());
    settings.setValue("fontColb", fontColor.blue());
    settings.setValue("lastSaveStr", lastSaveStr);
    settings.setValue("lastIconStr", lastIconStr);
    settings.setValue("lastOpenDir", lastOpenDir);
    settings.setValue("lastImportExportStr", lastImportExportStr);
    settings.setValue("sheetFont", sheetFont.toString());
    settings.setValue("animNames", ui->animNameEnabled->isChecked());
    settings.setValue("lastGIFStr", lastGIFStr);
    settings.setValue("minimizeSheetWidth", ui->minWidthCheckbox->isChecked());
    //settings.setValue("", );
}

void MainWindow::loadSettings()
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
    if(settings.value("fontColr", -1).toInt() != -1)
    {
        fontColor.setRed(settings.value("fontColr").toInt());
        fontColor.setGreen(settings.value("fontColg").toInt());
        fontColor.setBlue(settings.value("fontColb").toInt());
    }
    ui->minWidthCheckbox->setChecked(settings.value("minimizeSheetWidth", true).toBool());
    lastSaveStr = settings.value("lastSaveStr").toString();
    lastIconStr = settings.value("lastIconStr").toString();
    lastOpenDir = settings.value("lastOpenDir").toString();
    lastImportExportStr = settings.value("lastImportExportStr").toString();
    QString sFontVal = settings.value("sheetFont").toString();
    if(!sFontVal.size())
        sheetFont = QFont("MS Shell Dlg 2", 8);
    else
        sheetFont.fromString(sFontVal);
    if(settings.value("animNames").isValid())
        ui->animNameEnabled->setChecked(settings.value("animNames").toBool());
    if(settings.value("lastGIFStr").isValid())
        lastGIFStr = settings.value("lastGIFStr").toString();

    //Done reading settings; init program state

    //Fill in frame/sheet colors
    setColorButtonIcons();

    ui->frameBgColSelect->setEnabled(!ui->FrameBgTransparent->isChecked());
    ui->sheetBgColSelect->setEnabled(!ui->SheetBgTransparent->isChecked());

    //Init sheet values
    if(sheet)
    {
        sheet->setXSpacing(ui->xSpacingBox->value());
        sheet->setYSpacing(ui->ySpacingBox->value());
        sheet->setWidth(ui->sheetWidthBox->value());
        sheet->setBgCol(sheetBgCol);
        sheet->setFrameBgCol(frameBgCol);
        sheet->setBgTransparent(ui->SheetBgTransparent->isChecked());
        sheet->setFrameBgTransparent(ui->FrameBgTransparent->isChecked());
    }

    bLoadMutex = false;
}

void MainWindow::on_sheetWidthBox_valueChanged(int arg1)
{
    if(sheet)
    {
        unsigned int smallestPossible = sheet->getSmallestPossibleWidth();
        if((unsigned int) arg1 < smallestPossible)
            arg1 = smallestPossible;
        sheet->setWidth(arg1);
    }
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
            saveFile();
            if(bFileModified)   //If they still haven't saved...
                return;
        }
        else if(dialog != QMessageBox::Discard)
            return;
    }

    //TODO wipe current sheet

    //    if(mCurSheet)
    //        delete mCurSheet;
    //    mCurSheet = NULL;

    //    mSheetFrames.clear();
    //    mCurAnim = mSheetFrames.begin();
    //    mAnimNames.clear();
    //    mCurAnimName = mAnimNames.begin();

    drawAnimation();

    sCurFilename = UNTITLED_IMAGE_STR;
    bFileModified = false;
    updateWindowTitle();

    //Store file orig state, clear undo/redo
    clearUndo();
    clearRedo();
    pushUndo();
    updateUndoRedoMenu();
}

//TODO Combine with eventFilter
void MainWindow::keyPressEvent(QKeyEvent* e)
{
    Q_UNUSED(e)

    //TODO allow deleting currently selected frames/animations

    //Deleting current selected frame
    //    if(e->key() == Qt::Key_Delete && mSheetFrames.size() && mCurAnim != mSheetFrames.end() && mCurSelected != mCurAnim->end() && mCurSelectedInAnim != mSheetFrames.end())
    //    {
    //        mCurSelectedInAnim->erase(mCurSelected);
    //        mCurSelected = mCurSelectedInAnim->end();

    //        //Test and see if this anim is now empty
    //        if(!mCurSelectedInAnim->size())
    //        {
    //            mCurAnim = mCurSelectedInAnim;
    //            on_removeAnimButton_clicked();  //Simulate deleting this anim
    //        }

    //        mouseCursorPos(curMouseX, curMouseY);   //Select again
    //        //drawSheet();
    //        genUndoState();
    //    }
}

void MainWindow::on_saveFrameButton_clicked()
{
    //TODO Set image for saving TSR icon

    if(!sheet || !sheet->size())
        return;

    //    setIconImage(*mCurFrame);
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
                //                if(mCurAnim != mSheetFrames.begin() && mCurAnimName != mAnimNames.begin())
                //                {
                //                    mCurAnim--;
                //                    mCurAnimName--;
                //                }

                //                //drawSheet();

                //                if(mCurAnimName != mAnimNames.end())
                //                    ui->animationNameEditor->setText(*mCurAnimName);
                //                else
                //                    ui->animationNameEditor->setText(QString(""));

                //                if(mCurAnim != mSheetFrames.end())
                //                    mCurFrame = mCurAnim->begin();

                drawAnimation();

                ui->animationNameEditor->selectAll();
                genUndoState();
                return true;
            }
            else if(keyEvent->key() == Qt::Key_Down)
            {
                //Move forward one anim
                //                if(mCurAnim != mSheetFrames.end() && mCurAnimName != mAnimNames.end())
                //                {
                //                    mCurAnim++;
                //                    mCurAnimName++;
                //                    if(mCurAnim == mSheetFrames.end() || mCurAnimName == mAnimNames.end())
                //                    {
                //                        mCurAnim--;
                //                        mCurAnimName--;
                //                    }
                //                }

                //drawSheet();

                //                if(mCurAnimName != mAnimNames.end())
                //                    ui->animationNameEditor->setText(*mCurAnimName);
                //                else
                //                    ui->animationNameEditor->setText(QString(""));

                //                if(mCurAnim != mSheetFrames.end())
                //                    mCurFrame = mCurAnim->begin();

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

void MainWindow::on_fontColSelect_clicked()
{
    QColor selected = colorSelect.getColor(fontColor, this, "Select Font Color");
    if(selected.isValid())
    {
        fontColor = selected;
        QPixmap colIcon(32, 32);
        colIcon.fill(fontColor);
        QIcon ic(colIcon);
        ui->fontColSelect->setIcon(ic);
        //TODO Update font color of labels
        drawAnimation();
        genUndoState();
    }
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
        if(sheet)
            sheet->setFrameBgCol(selected);
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
        genUndoState();
        if(sheet)
            sheet->setBgCol(selected);
    }
}

void MainWindow::on_FrameBgTransparent_toggled(bool checked)
{
    if(bLoadMutex)
        return;

    ui->frameBgColSelect->setEnabled(!checked);
    drawAnimation();
    genUndoState();
    if(sheet)
        sheet->setFrameBgTransparent(checked);
}

void MainWindow::on_SheetBgTransparent_toggled(bool checked)
{
    if(bLoadMutex)
        return;

    ui->sheetBgColSelect->setEnabled(!checked);
    if(sheet)
        sheet->setBgTransparent(checked);
}

void MainWindow::on_balanceAnimButton_clicked()
{
    if(!sheet || !sheet->size())
        return;

    Animation* anim = sheet->getAnimation(sheet->size()-1); //TODO Anim select
    if(!anim)
        return;

    QPoint curAnimSz = anim->getMaxFrameSize();
    setBalanceDefWH(curAnimSz.x(), curAnimSz.y());

    mBalanceWindow->show();
    centerParent(this, mBalanceWindow);
}

void MainWindow::balance(int w, int h, BalancePos::Pos vert, BalancePos::Pos horiz)
{
    qDebug() << "enter function balance()" << endl;

    if(!sheet || !sheet->size())
        return;

    Animation* anim = sheet->getAnimation(sheet->size()-1); //TODO Anim select
    if(!anim)
        return;

    anim->balance(QPoint(w,h), vert, horiz);
    sheet->refresh();

    //TODO Update animation frame preview
    //    mCurFrame = mCurAnim->begin();
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
        if(!sheet || !sheet->size())
            return;

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
            s.setVersion(QDataStream::Qt_5_6);
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
            saveFile();
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
            s.setVersion(QDataStream::Qt_5_6);

            //Clean up memory
            cleanMemory();

            loadFromStream(s);

            QFileInfo fi(openFilename);
            sCurFilename = fi.fileName();
            bFileModified = false;
            updateWindowTitle();
            lastSaveStr = openFilename;
            clearUndo();
            clearRedo();
            pushUndo();
            updateUndoRedoMenu();
        }
    }
}

void MainWindow::saveToStream(QDataStream& s)
{
    //TODO Add stream saving to Sheet class

    //Save sheet frames
    int curAnim = 0, curFrame = 0;
    //int cnt = 0;
    int major = MAJOR_VERSION;
    int minor = MINOR_VERSION;
    int rev = REV_VERSION;
    s << major << minor << rev;  //Later we'll care about this if the save format changes again
    //s << mSheetFrames.size();
    //    for(QList<QList<QImage> >::iterator i = mSheetFrames.begin(); i != mSheetFrames.end(); i++)
    //    {
    //        if(i == mCurAnim)
    //            curAnim = cnt;
    //        cnt++;

    //        s << i->size();
    //        int innercnt = 0;
    //        for(QList<QImage>::iterator j = i->begin(); j != i->end(); j++)
    //        {
    //            if(i == mCurAnim && j == mCurFrame)
    //                curFrame = innercnt;
    //            innercnt++;

    //            QByteArray imgByteArray;
    //            QBuffer buffer(&imgByteArray);
    //            buffer.open(QIODevice::WriteOnly);
    //            j->save(&buffer, "TIFF");
    //            s << imgByteArray;
    //        }
    //    }

    //Save anim names
    //s << mAnimNames.size();
    //foreach(QString str, mAnimNames)
    //    s << str;

    //Save other stuff
    s << sheetBgCol;
    s << frameBgCol;
    s << fontColor;
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
    s >> major >> minor >> rev;
    if(major > MAJOR_VERSION || (major == MAJOR_VERSION && minor > MINOR_VERSION) || (major == MAJOR_VERSION && minor == MINOR_VERSION && rev > REV_VERSION))
    {
        QMessageBox::warning(this, "File Load", "This sheet file was created with a newer version of Sprite Sheeter than you currently have. Please update your Sprite Sheeter version");
        return;
    }
    int numAnims = 0;
    s >> numAnims;
    for(int i = 0; i < numAnims; i++)
    {
        QList<QImage> imgList;
        int numFrames = 0;
        s >> numFrames;
        for(int j = 0; j < numFrames; j++)
        {
            //Load from TIFF
            QImage img;
            QByteArray imgByteArray;
            s >> imgByteArray;
            QBuffer buffer(&imgByteArray);
            buffer.open(QIODevice::ReadOnly);
            img.load(&buffer, "TIFF");
            imgList.push_back(img);
        }
        //mSheetFrames.push_back(imgList);
    }

    //Grab anim names
    int numAnimNames = 0;
    s >> numAnimNames;
    for(int i = 0; i < numAnimNames; i++)
    {
        QString str;
        s >> str;
        //mAnimNames.push_back(str);
    }

    //Read other stuff
    s >> sheetBgCol;
    s >> frameBgCol;
    if(major > 1 || (major == 1 && minor > 1))  //Version 1.2 introduced font color
        s >> fontColor;

    //Fill in frame/sheet colors
    setColorButtonIcons();

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
    //    mCurAnim = mSheetFrames.begin();
    //    mCurAnimName = mAnimNames.begin();
    //    if(mCurAnim != mSheetFrames.end())
    //        mCurFrame = mCurAnim->begin();

    //Set to correct anim and frame...
    int cnt = 0;
    //    for(QList<QList<QImage> >::iterator i = mSheetFrames.begin(); i != mSheetFrames.end(); i++, cnt++)
    //    {
    //        if(cnt == curAnim)
    //        {
    //            mCurFrame = i->begin();
    //            mCurAnim = i;
    //            int innercnt = 0;
    //            for(QList<QImage>::iterator j = i->begin(); j != i->end(); j++, innercnt++)
    //            {
    //                if(innercnt == curAnimFrame)
    //                {
    //                    mCurFrame = j;
    //                    break;
    //                }
    //            }
    //            break;
    //        }
    //    }

    //Set to correct anim name...
    cnt = 0;
    //    for(QList<QString>::iterator i = mAnimNames.begin(); i != mAnimNames.end(); i++, cnt++)
    //    {
    //        if(cnt == curAnim)
    //        {
    //            mCurAnimName = i;
    //            break;
    //        }
    //    }

    //Reset GUI stuff!
    //    if(mCurAnimName != mAnimNames.end())
    //        ui->animationNameEditor->setText(*mCurAnimName);

    drawAnimation();
    bLoadMutex = false;
}

void MainWindow::cleanMemory()
{
    //TODO Wipe sheet

    //    if(mCurSheet)
    //        delete mCurSheet;
    //    mCurSheet = NULL;

    //    mSheetFrames.clear();
    //    mCurAnim = mSheetFrames.begin();
    //    mAnimNames.clear();
    //    mCurAnimName = mAnimNames.begin();
}

void MainWindow::on_fontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, sheetFont, this);
    if(ok)
    {
        sheetFont = font;
        //TODO Update font in labels
        genUndoState();
    }
}

void MainWindow::updateWindowTitle()
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
    if((!sheet || !sheet->size()) && sCurFilename == UNTITLED_IMAGE_STR && undoList.size() < 2)
    {
        clearUndo();
        pushUndo(); //Save this as our starting state
        return;   //Don't generate undo states on empty sheet
    }

    //Set the window title if this is the first the file has been modified
    if(!bFileModified)
    {
        bFileModified = true;
        updateWindowTitle();
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

void MainWindow::on_xSpacingBox_editingFinished()
{
    if(ui->minWidthCheckbox->isChecked())
        minimizeSheetWidth();
    genUndoState();
}

void MainWindow::on_ySpacingBox_editingFinished()
{
    genUndoState();
}

void MainWindow::on_sheetWidthBox_editingFinished()
{
    if(ui->minWidthCheckbox->isChecked())
        minimizeSheetWidth();
    sheet->updateSceneBounds();
    genUndoState();
}

void MainWindow::on_animationNameEditor_editingFinished()
{
    genUndoState();
}

void MainWindow::on_animNameEnabled_toggled(bool checked)
{
    if(checked)
    {
        ui->animationNameEditor->setEnabled(true);
        ui->fontButton->setEnabled(true);
        ui->fontColSelect->setEnabled(true);
    }
    else
    {
        ui->animationNameEditor->setEnabled(false);
        ui->fontButton->setEnabled(false);
        ui->fontColSelect->setEnabled(false);
    }
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
                //HACK Quantize low-alpha to magenta by hand...
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

//See http://sourceforge.net/p/freeimage/discussion/36111/thread/ea987d97/ for discussion of FreeImage gif saving...
void MainWindow::on_ExportAnimButton_clicked()
{
    if(!sheet || !sheet->size())
        return;

    QString saveFilename = QFileDialog::getSaveFileName(this,
                                                        tr("Save GIF Animation"),
                                                        lastGIFStr,
                                                        tr("GIF Image (*.gif)"));

    if(saveFilename.length())
    {
        lastGIFStr = saveFilename;

        //Open GIF image for writing
        FIMULTIBITMAP* bmp = FreeImage_OpenMultiBitmap(FIF_GIF, saveFilename.toStdString().c_str(), true, false);

        //TODO Add GIF saving to somewhere not here. Likely Animation class or some other class entirely

        //DWORD dwFrameTime = (DWORD)((1000.0f / (float)ui->animationSpeedSpinbox->value()) + 0.5f); //Framerate of gif image

        //        for(QList<QImage>::iterator i = mCurAnim->begin(); i != mCurAnim->end(); i++)
        //        {
        //            //Create image and 256-color image
        //            QImage imgTemp(*i);
        //            //Gotta get Qt image in proper format first
        //            imgTemp = imgTemp.convertToFormat(QImage::Format_ARGB32);
        //            QByteArray bytes((char*)imgTemp.bits(), imgTemp.byteCount());
        //            //HACK Make 32-bit image with magenta instead of transparency first...
        //            FIBITMAP* page = imageFromPixels((uint8_t*)bytes.data(), imgTemp.width(), imgTemp.height());
        //            //Turn this into an 8-bit image next
        //            FIBITMAP* page8bit = FreeImage_ColorQuantize(page, FIQ_WUQUANT);

        //            //Set transparency table from magenta. *Hopefully this was preserved during quantization!*
        //            RGBQUAD *Palette = FreeImage_GetPalette(page8bit);
        //            BYTE Transparency[256];
        //            for (unsigned i = 0; i < 256; i++)
        //            {
        //                Transparency[i] = 0xFF;
        //                if(Palette[i].rgbGreen == 0x00 &&
        //                        Palette[i].rgbBlue == 0xFF &&
        //                        Palette[i].rgbRed == 0xFF)
        //                {
        //                    Transparency[i] = 0x00;
        //                }
        //            }
        //            FreeImage_SetTransparencyTable(page8bit, Transparency, 256);

        //            //Append metadata - frame speed based on current playback speed
        //            FreeImage_SetMetadata(FIMD_ANIMATION, page8bit, NULL, NULL);
        //            FITAG *tag = FreeImage_CreateTag();
        //            if(tag)
        //            {
        //                FreeImage_SetTagKey(tag, "FrameTime");
        //                FreeImage_SetTagType(tag, FIDT_LONG);
        //                FreeImage_SetTagCount(tag, 1);
        //                FreeImage_SetTagLength(tag, 4);
        //                FreeImage_SetTagValue(tag, &dwFrameTime);
        //                FreeImage_SetMetadata(FIMD_ANIMATION, page8bit, FreeImage_GetTagKey(tag), tag);
        //                FreeImage_DeleteTag(tag);
        //            }
        //            FreeImage_AppendPage(bmp, page8bit);
        //            FreeImage_Unload(page);
        //            FreeImage_Unload(page8bit);
        //        }

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
    QVector<QImage*> frameList;
    for(int i = 0; i < numFrames; i++)
    {
        FIBITMAP* frame = FreeImage_LockPage(bmp, i);
        FIBITMAP* frame32bit = FreeImage_ConvertTo32Bits(frame);

        QImage imgResult(FreeImage_GetBits(frame32bit), FreeImage_GetWidth(frame32bit), FreeImage_GetHeight(frame32bit), FreeImage_GetPitch(frame32bit), QImage::Format_ARGB32);
        frameList.push_back(new QImage(imgResult.mirrored()));

        FreeImage_Unload(frame32bit);   //Qt expects the memory to be available the whole time wut? Luckily, we have to mirror it anyway so it doesn't matter
        FreeImage_UnlockPage(bmp, frame, false);
    }

    QString fileName = QFileInfo(sFilename).baseName();
    insertAnimHelper(frameList, fileName);

    drawAnimation();
    genUndoState();

    FreeImage_CloseMultiBitmap(bmp);
    return true;
}

void MainWindow::on_reverseAnimButton_clicked()
{
    if(sheet)
    {
        Animation* anim = sheet->getAnimation(sheet->size()-1);    //TODO Selected anim
        if(anim)
        {
            anim->reverse();
            sheet->refresh();   //Tell sheet to recalculate positions
        }
    }

    drawAnimation();
    genUndoState();
}

void MainWindow::on_removeDuplicateFramesButton_clicked()
{
    if(sheet && sheet->size())
    {
        Animation* anim = sheet->getAnimation(sheet->size()-1);   //TODO Selected anim
        if(anim)
        {
            if(anim->removeDuplicateFrames())
                sheet->refresh();
        }

        //mCurFrame = mCurAnim->begin();
        //drawAnimation();
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
            "<p>Questions/comments? Ping me on <a href=\"https://www.vg-resource.com/thread-29374-post-625345.html\">the Spriters Resource forum!</a></p>";
    QMessageBox::about(this, "About Sprite Sheeter", aboutText);
}

void MainWindow::on_actionBatch_Processing_triggered()
{
    //Don't start more than one batch job at a time
    if(progressBar != NULL)
        return;

    //HACK The standard Windows dialogs don't allow selection of multiple folders; make one manually
    QFileDialog* multiSelectFolder = new QFileDialog(this, "Select folders for batch processing");
    multiSelectFolder->setWindowFlags(multiSelectFolder->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    multiSelectFolder->setFileMode(QFileDialog::DirectoryOnly);
    multiSelectFolder->setOption(QFileDialog::DontUseNativeDialog, true);
    multiSelectFolder->setDirectory(lastOpenDir);
    QListView *listView = multiSelectFolder->findChild<QListView*>("listView");
    if(listView)
        listView->setSelectionMode(QAbstractItemView::MultiSelection);
    QTreeView *treeView = multiSelectFolder->findChild<QTreeView*>();
    if(treeView)
        treeView->setSelectionMode(QAbstractItemView::MultiSelection);
    if(!multiSelectFolder->exec())
    {
        delete multiSelectFolder;
        return;
    }
    QStringList fileNames = multiSelectFolder->selectedFiles();
    delete multiSelectFolder;

    //Trim off first item if it's the folder above the others (bug in file dialog's return value)
    if(fileNames.size() > 1)
    {
        QString firstFolder = fileNames.at(0);
        lastOpenDir = firstFolder;  //Hold onto this for next time
        QString firstFolderFS = firstFolder + '/';
        QString firstFolderBS = firstFolder + '\\';
        QString secondFolder = fileNames.at(1);

        if(secondFolder.indexOf(firstFolderFS) != -1 || secondFolder.indexOf(firstFolderBS) != -1)
            fileNames.pop_front();
    }

    //Stop here if there are none
    if(!fileNames.size())
        return;

    //Create progress bar dialog
    progressBar = new QProgressDialog("Starting...", "Cancel", 0, fileNames.size()-1);
    progressBar->setWindowTitle("Batch Rendering");
    //Hide that stupid help button
    progressBar->setWindowFlags(progressBar->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    //Spin off threads to render these
    foreach(QString folder, fileNames)
    {
        BatchRenderer* batchRenderer = new BatchRenderer();
        batchRenderer->folder = folder;
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
        batchRenderer->fontColor = fontColor;

        QObject::connect(batchRenderer, SIGNAL(renderingStart(QString)), this, SLOT(startedBatchRender(QString)));
        QObject::connect(batchRenderer, SIGNAL(renderingDone()), this, SLOT(finishedBatchRender()));
        QObject::connect(progressBar, SIGNAL(canceled()), batchRenderer, SLOT(stop()));

        QThreadPool::globalInstance()->start(batchRenderer);
    }
    //Clean up your own memory bro
    progressBar->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(progressBar, SIGNAL(canceled()), this, SLOT(threadRenderingCanceled()));
    progressBar->show();
}

void MainWindow::startedBatchRender(QString sheetName)
{
    if(progressBar)
        progressBar->setLabelText(sheetName);
}

void MainWindow::finishedBatchRender()
{
    if(!progressBar) return;

    //once progress bar is set to maximum, import is done, so test this first
    if(progressBar->value()+1 >= progressBar->maximum())
    {
        progressBar->setValue(progressBar->value()+1);
        progressBar = NULL;
    }
    else
        progressBar->setValue(progressBar->value()+1);
}

void MainWindow::threadRenderingCanceled()
{
    QThreadPool::globalInstance()->clear(); //Don't start new ones
    progressBar = NULL;
}

void MainWindow::setColorButtonIcons()
{
    QPixmap colIcon(32, 32);

    colIcon.fill(sheetBgCol);
    ui->sheetBgColSelect->setIcon(QIcon(colIcon));

    colIcon.fill(frameBgCol);
    ui->frameBgColSelect->setIcon(QIcon(colIcon));

    colIcon.fill(fontColor);
    ui->fontColSelect->setIcon(QIcon(colIcon));
}

void MainWindow::on_minWidthCheckbox_toggled(bool checked)
{
    if(sheet && checked)
        minimizeSheetWidth();
}

void MainWindow::minimizeSheetWidth()
{
    unsigned int width = sheet->getMinWidth();
    ui->sheetWidthBox->setValue(width); //Updates width of sheet automatically
}
