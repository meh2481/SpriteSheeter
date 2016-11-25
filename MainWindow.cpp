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
#include "undo/FontColorStep.h"
#include "undo/FrameBgColorStep.h"
#include "undo/SheetBgColorStep.h"
#include "undo/SheetBgTransparentStep.h"
#include "undo/FrameBgTransparentStep.h"
#include "undo/SheetFontStep.h"
#include "undo/YSpacingStep.h"
#include "undo/XSpacingStep.h"

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

    ui->animStopButton->setVisible(false);

    //TODO Implement cut/copy/paste
    ui->cutCopyPasteSeparator->setVisible(false);
    ui->cutButton->setVisible(false);
    ui->copyButton->setVisible(false);
    ui->pasteButton->setVisible(false);
    ui->actionCut->setVisible(false);
    ui->actionCopy->setVisible(false);
    ui->actionPaste->setVisible(false);

    //Connect all our signals & slots up
    QObject::connect(mImportWindow, SIGNAL(importOK(int, int, bool, bool)), this, SLOT(importNext(int, int, bool, bool)));
    QObject::connect(mImportWindow, SIGNAL(importAll(int, int, bool, bool)), this, SLOT(importAll(int, int, bool, bool)));
    QObject::connect(this, SIGNAL(setImportImg(QImage*)), mImportWindow, SLOT(setPreviewImage(QImage*)));
    QObject::connect(ui->sheetPreview, SIGNAL(mouseMoved(int,int)), this, SLOT(mouseCursorPos(int, int)));
    QObject::connect(ui->sheetPreview, SIGNAL(mousePressed(int,int)), this, SLOT(mouseDown(int, int)));
    QObject::connect(ui->sheetPreview, SIGNAL(mouseReleased(int,int)), this, SLOT(mouseUp(int, int)));
    QObject::connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
    QObject::connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(saveFileAs()));
    QObject::connect(ui->actionImport_WIP_Sheet, SIGNAL(triggered(bool)), this, SLOT(loadSheet()));
    QObject::connect(ui->actionUndo, SIGNAL(triggered(bool)), this, SLOT(undo()));
    QObject::connect(ui->actionRedo, SIGNAL(triggered(bool)), this, SLOT(redo()));
    QObject::connect(ui->actionCut, SIGNAL(triggered(bool)), this, SLOT(cut()));
    QObject::connect(ui->actionCopy, SIGNAL(triggered(bool)), this, SLOT(copy()));
    QObject::connect(ui->actionPaste, SIGNAL(triggered(bool)), this, SLOT(paste()));
    QObject::connect(ui->sheetPreview, SIGNAL(droppedFiles(QStringList)), this, SLOT(addImages(QStringList)));
    QObject::connect(ui->sheetPreview, SIGNAL(droppedFolders(QStringList)), this, SLOT(addFolders(QStringList)));
    QObject::connect(mBalanceWindow, SIGNAL(balance(int,int,BalancePos::Pos,BalancePos::Pos)), this, SLOT(balance(int,int,BalancePos::Pos,BalancePos::Pos)));
    QObject::connect(this, SIGNAL(setBalanceDefWH(int,int)), mBalanceWindow, SLOT(defaultWH(int,int)));
    QObject::connect(this, SIGNAL(setIconImage(QImage)), mIconExportWindow, SLOT(setImage(QImage)));
    QObject::connect(mRecentDocuments, SIGNAL(openFile(QString)), this, SLOT(loadSheet(QString)));

    animItem = NULL;
    progressBar = NULL;
    mCurFrame = NULL;
    mAnimFrame = 0;
    clicked = selected = lastSelected = NULL;
    transparentBg = new QImage("://bg");
    bUIMutex = false;

    bDraggingSheetW = false;
    m_bDraggingSelected = false;
    m_bSetDraggingCursor = false;
    setModified(false);
    m_rLastDragHighlight.setCoords(0,0,0,0);
    m_bLastDragInAnim = false;
    sCurFilename = UNTITLED_IMAGE_STR;

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

    animHighlightCol = QColor(128, 0, 0, 255);

    //Create animation sheet
    sheet = new Sheet(msheetScene, ui->sheetPreview, transparentBg, DRAG_HANDLE_SIZE);

    if(ui->sheetPreview->isHidden())
        ui->sheetPreview->show();

    QPen linePen(QColor(0,0,255), SELECT_RECT_THICKNESS);
    curSelectedRect = msheetScene->addRect(QRect(0,0,0,0), linePen);
    curSelectedRect->setZValue(2); //Above most everything
    curSelectedRect->setVisible(false);

    curDragLine = msheetScene->addLine(0, 0, 10, 10, linePen);
    curDragLine->setZValue(2);
    curDragLine->setVisible(false);

    curSelectedAnimRect = msheetScene->addRect(0,0,0,0,QPen(),QBrush(QColor(128, 0, 0, 255)));
    curSelectedAnimRect->setZValue(-3);
    curSelectedAnimRect->setVisible(true);

    //Read in settings here
    loadSettings();

    //Set color icons to proper color
    setColorButtonIcons();
    updateWindowTitle();

    //Store initial undo state
    //pushUndo();
    updateUndoRedoMenu();

    msheetScene->setSceneRect(0,0,0,0);
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
    if(fileList.size())
    {
        Animation* animation = new Animation(transparentBg, msheetScene, this);
        foreach(QString s1, fileList)
        {
            QString imgPath = prepend + s1;
            QImage* image = loadImageFI(imgPath);
            if(!image->isNull())
                animation->insertImage(image);
            else
                qDebug() << "Unable to open image " << imgPath << endl;
        }
        animation->setName(animName);
        sheet->addAnimation(animation);
        checkMinWidth();

        if(ui->minWidthCheckbox->isChecked())
            minimizeSheetWidth();
        drawAnimation();
        updateSelectedAnim();
    }
}

QStringList MainWindow::supportedFileFormats()
{
    QStringList fileFilters;
    //TODO: Use FreeImage supported file formats
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
    //if(l.size())
        //genUndoState();
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
        importImageList(mOpenFiles);
        //genUndoState();
    }
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
    if(setImportImg(loadImageFI(curImportImage)))
    {
        mImportWindow->show();
        //Center on parent
        centerParent(this, mImportWindow);
    }
}

void MainWindow::insertAnimHelper(QVector<QImage*> imgList, QString name)
{
    if(imgList.size())
    {
        Animation* animation = new Animation(transparentBg, msheetScene, this);
        animation->insertImages(imgList);
        animation->setName(name);
        sheet->addAnimation(animation, sheet->getCurSelected());
        updateSelectedAnim();
    }
}

void MainWindow::importImageAsSheet(QString s, int numxframes, int numyframes, bool bVert, bool bSplit)
{
    QImage* image = loadImageFI(s);
    if(!image)
    {
        QMessageBox::information(this, "Image Import", "Error opening image " + s);
        return;
    }
    QString fileName = QFileInfo(s).baseName();

    //Find image dimensions
    int iXFrameSize = image->width() / numxframes;
    int iYFrameSize = image->height() / numyframes;

    //Grab all the frames out
    QVector<QImage*> imgList;
    if(!bVert)
    {
        for(int y = 0; y < numyframes; y++)
        {
            for(int x = 0; x < numxframes; x++)
            {
                imgList.push_back(new QImage(image->copy(x*iXFrameSize, y*iYFrameSize, iXFrameSize, iYFrameSize)));
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
                imgList.push_back(new QImage(image->copy(x*iXFrameSize, y*iYFrameSize, iXFrameSize, iYFrameSize)));
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

    delete image;
    checkMinWidth();
    drawAnimation();
    //genUndoState();
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
    sheet->setXSpacing(arg1);
    int minW = sheet->getSmallestPossibleWidth();
    if(minW > ui->sheetWidthBox->value())
        ui->sheetWidthBox->setValue(minW);
    updateSelectedAnim();
}

void MainWindow::on_ySpacingBox_valueChanged(int arg1)
{
    sheet->setYSpacing(arg1);
    sheet->refresh();
    sheet->updateSceneBounds();
    updateSelectedAnim();
}

void MainWindow::genericSave(QString saveFilename)
{
    if(saveFilename.length())
    {
        //Set cursor to saving cursor
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();
        statusBar()->showMessage("Saving " + saveFilename + "...");

        //Create image and save
        lastSaveStr = saveFilename;
        if(saveFilename.contains(".sheet", Qt::CaseInsensitive))
        {
            saveSheet(saveFilename);
            QFileInfo fi(saveFilename);
            sCurFilename = fi.fileName();
            setModified(false);
            updateWindowTitle();
        }
        else
        {
            if(!sheet->render(saveFilename))
            {
                QMessageBox::information(this,"Image Export","Error saving image " + saveFilename);
            }
            else
            {
                QFileInfo fi(saveFilename);
                sCurFilename = fi.fileName();
                setModified(false);
                updateWindowTitle();
            }
        }

        //Set cursor back
        QApplication::restoreOverrideCursor();
        statusBar()->showMessage(saveFilename + " saved!");
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

void MainWindow::on_animationNameEditor_textChanged(const QString& arg1)
{
    if(sheet->size())
    {
        Animation* anim = sheet->getAnimation(sheet->getCurSelected());
        anim->setName(arg1);
        sheet->refresh();
        sheet->updateSceneBounds();
        updateSelectedAnim();
    }
}

void MainWindow::drawAnimation()
{
    mCurFrame = NULL;
    if(sheet->size())
    {
        Animation* anim = sheet->getAnimation(sheet->getCurSelected());
        QVector<Frame*> frames = anim->getFrames();
        if(frames.size())
        {
            if(mAnimFrame > frames.size()-1)
                mAnimFrame = frames.size()-1;
            mCurFrame = frames.at(mAnimFrame)->getImage();
        }
    }

    if(!mCurFrame)
    {
        if(animItem)
            animItem->hide();
        ui->curFrameLabel->setText("Current Frame: 0/0");
        return;
    }

    //Draw image and bg
    QImage animFrame(mCurFrame->width(), mCurFrame->height(), QImage::Format_ARGB32);
    QPainter painter(&animFrame);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    if(ui->frameBgTransparent->isChecked())
    {
        QBrush bgTexBrush(*transparentBg);
        painter.fillRect(0, 0, mCurFrame->width(), mCurFrame->height(), bgTexBrush);
    }
    else
        animFrame.fill(sheet->getFrameBgCol());

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
    int numFramesTotal = 0;
    if(sheet->getCurSelected() >= 0 && sheet->getCurSelected() < (int)sheet->size())
        numFramesTotal = sheet->getAnimation(sheet->getCurSelected())->getFrames().size();
    ui->curFrameLabel->setText("Current Frame: " + QString::number(mAnimFrame+1) + "/" + QString::number(numFramesTotal));
}

void MainWindow::animUpdate()
{
    if(sheet->size())
    {
        Animation* anim = sheet->getAnimation(sheet->getCurSelected());
        QVector<Frame*> frames = anim->getFrames();
        if(frames.size())
        {
            mAnimFrame++;
            if(mAnimFrame > frames.size()-1)
                mAnimFrame = 0;
        }
    }

    drawAnimation();
}

void MainWindow::on_animationSpeedSpinbox_valueChanged(int arg1)
{
    int iInterval = 1000/arg1;
    animUpdateTimer->stop();
    animUpdateTimer->start(iInterval);
    updatePlayIcon();
}

void MainWindow::on_animPlayButton_clicked()
{
    if(!animUpdateTimer->isActive())
    {
        int iInterval = 1000/ui->animationSpeedSpinbox->value();
        animUpdateTimer->start(iInterval);
    }
    else
    {
        animUpdateTimer->stop();
    }
    updatePlayIcon();
}

void MainWindow::on_animStopButton_clicked()
{
    animUpdateTimer->stop();
    mAnimFrame = 0;
    drawAnimation();
    updatePlayIcon();
}

void MainWindow::on_animPrevFrameButton_clicked()
{
    if(animUpdateTimer->isActive())
        animUpdateTimer->stop();
    if(sheet->size())
    {
        Animation* anim = sheet->getAnimation(sheet->getCurSelected());
        QVector<Frame*> frames = anim->getFrames();
        if(frames.size())
        {
            mAnimFrame--;
            if(mAnimFrame > frames.size()-1)
                mAnimFrame = frames.size()-1;
            if(mAnimFrame < 0)
                mAnimFrame = frames.size()-1;
        }
    }

    drawAnimation();
    updatePlayIcon();
}

void MainWindow::on_animNextFrameButton_clicked()
{
    if(animUpdateTimer->isActive())
        animUpdateTimer->stop();
    if(sheet->size())
    {
        Animation* anim = sheet->getAnimation(sheet->getCurSelected());
        QVector<Frame*> frames = anim->getFrames();
        if(frames.size())
        {
            mAnimFrame++;
            if(mAnimFrame > frames.size()-1)
                mAnimFrame = 0;
        }
    }

    drawAnimation();
    updatePlayIcon();
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
    bool bSelectRectVisible = false;
    if(sheet)
    {
        //Update cursor if need be
        if(isMouseOverDragArea(x, y) && !selected)
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
        else
        {
            if(!selected)
            {
                QGraphicsItem* it = isItemUnderCursor(x, y);
                if(it != NULL)
                {
                    //Draw box around currently-highlighted image
                    curSelectedRect->setRect(it->boundingRect());
                    curSelectedRect->setPos(it->x(), it->y());
                    bSelectRectVisible = true;
                }
            }
            else
            {
                QLine pos = sheet->getDragPos(x, y);
                //qDebug() << "Drag pos" << pos;
                if(pos.x1() >= 0 && pos.y1() >= 0)
                {
                    //qDebug() << "set line";
                    curDragLine->setLine(pos);
                    curDragLine->setVisible(true);
                }
                else
                    curDragLine->setVisible(false);
            }
        }
    }
    curSelectedRect->setVisible(bSelectRectVisible);

    //Show the mouse cursor pos
    statusBar()->showMessage(QString::number(x) + ", " + QString::number(y));
    curMouseY = y;
    curMouseX = x;
}

void MainWindow::mouseDown(int x, int y)
{
    selected = NULL;
    if(sheet)
    {
        //We're starting to drag the sheet size handle
        if(isMouseOverDragArea(x, y))
        {
            bDraggingSheetW = true;
            mStartSheetW = sheet->getWidth();
            xStartDragSheetW = x;
        }
        else
        {
            clicked = isItemUnderCursor(x, y);
            if(clicked)
            {
                if(sheet->selected(clicked))
                    selected = clicked;
            }
        }
    }
}

void MainWindow::mouseUp(int x, int y)
{
    if(sheet)
    {
        if(bDraggingSheetW)
        {
            bDraggingSheetW = false;
            sheet->updateSceneBounds();
            //genUndoState();
            lastSheetW = ui->sheetWidthBox->value();
        }
        else
        {
            QGraphicsItem* itemUnder = isItemUnderCursor(x, y);
            if(clicked && itemUnder == clicked)
            {
                //Shift-click to select line
                if(lastSelected && QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
                {
                    sheet->selectLine(clicked, lastSelected);
                    sheet->clicked(x, y, clicked);  //Deselect so it can be reselected again (FSMs are hard)
                }
                //Ctrl-click to select multiple
                else if(!(QGuiApplication::keyboardModifiers() & Qt::ControlModifier))
                    sheet->deselectAll();

                if(sheet->clicked(x, y, clicked))
                    lastSelected = clicked;

                sheet->selectAnimation(sheet->getSelected(x, y));
            }
            else if(selected)
            {
                sheet->selectAnimation(sheet->getSelected(x, y));
                sheet->dropped(x, y);
            }
            else if(!itemUnder)
            {
                sheet->selectAnimation(sheet->getSelected(x, y));
                sheet->deselectAll();
            }
            updateSelectedAnim();
        }
    }
    selected = NULL;
    curDragLine->setVisible(false);
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
    settings.setValue("FrameBgTransparent", ui->frameBgTransparent->isChecked());
    settings.setValue("SheetBgTransparent", ui->sheetBgTransparent->isChecked());
    settings.setValue("sheetBgColr", sheet->getBgCol().red());
    settings.setValue("sheetBgColg", sheet->getBgCol().green());
    settings.setValue("sheetBgColb", sheet->getBgCol().blue());
    settings.setValue("frameBgColr", sheet->getFrameBgCol().red());
    settings.setValue("frameBgColg", sheet->getFrameBgCol().green());
    settings.setValue("frameBgColb", sheet->getFrameBgCol().blue());
    QColor fontColor = sheet->getFontColor();
    settings.setValue("fontColr", fontColor.red());
    settings.setValue("fontColg", fontColor.green());
    settings.setValue("fontColb", fontColor.blue());
    settings.setValue("lastSaveStr", lastSaveStr);
    settings.setValue("lastIconStr", lastIconStr);
    settings.setValue("lastOpenDir", lastOpenDir);
    settings.setValue("lastImportExportStr", lastImportExportStr);
    settings.setValue("sheetFont", sheet->getFont().toString());
    settings.setValue("animNames", ui->animNameEnabled->isChecked());
    settings.setValue("lastGIFStr", lastGIFStr);
    settings.setValue("minimizeSheetWidth", ui->minWidthCheckbox->isChecked());
    //settings.setValue("", );
}

void MainWindow::loadSettings()
{
    //Read in settings from config (registry or wherever it is)
    bUIMutex = true;
    QSettings settings("DaxarDev", "SpriteSheeter");
    if(settings.value("xSpacing", -1).toInt() == -1)    //No settings are here
        return;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->xSpacingBox->setValue(settings.value("xSpacing").toInt());
    ui->ySpacingBox->setValue(settings.value("ySpacing").toInt());
    ui->sheetWidthBox->setValue(settings.value("sheetWidth").toInt());
    ui->animationSpeedSpinbox->setValue(settings.value("animationSpeed").toInt());
    ui->frameBgTransparent->setChecked(settings.value("FrameBgTransparent").toBool());
    ui->sheetBgTransparent->setChecked(settings.value("SheetBgTransparent").toBool());
    QColor sheetBgCol;
    sheetBgCol.setRed(settings.value("sheetBgColr").toInt());
    sheetBgCol.setGreen(settings.value("sheetBgColg").toInt());
    sheetBgCol.setBlue(settings.value("sheetBgColb").toInt());
    QColor frameBgCol;
    frameBgCol.setRed(settings.value("frameBgColr").toInt());
    frameBgCol.setGreen(settings.value("frameBgColg").toInt());
    frameBgCol.setBlue(settings.value("frameBgColb").toInt());
    QColor fontColor;
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
    QFont sheetFont;
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

    ui->frameBgColSelect->setEnabled(!ui->frameBgTransparent->isChecked());
    ui->sheetBgColSelect->setEnabled(!ui->sheetBgTransparent->isChecked());

    //Init sheet values
    if(sheet)
    {
        sheet->setXSpacing(ui->xSpacingBox->value());
        sheet->setYSpacing(ui->ySpacingBox->value());
        sheet->setWidth(ui->sheetWidthBox->value());
        sheet->setBgCol(sheetBgCol);
        sheet->setFrameBgCol(frameBgCol);
        sheet->setBgTransparent(ui->sheetBgTransparent->isChecked());
        sheet->setFrameBgTransparent(ui->frameBgTransparent->isChecked());
        sheet->setFont(sheetFont);
        sheet->setFontColor(fontColor);
        sheet->updateSceneBounds();
    }

    lastYSpacing = ui->ySpacingBox->value();
    lastXSpacing = ui->xSpacingBox->value();
    lastSheetW = ui->sheetWidthBox->value();

    bUIMutex = false;
}

void MainWindow::on_sheetWidthBox_valueChanged(int arg1)
{
    if(sheet)
    {
        unsigned int smallestPossible = sheet->getSmallestPossibleWidth();
        if((unsigned int) arg1 < smallestPossible)
        {
            arg1 = smallestPossible;
            ui->sheetWidthBox->setValue(arg1);  //yaaay recursion
        }
        sheet->setWidth(arg1);
        updateSelectedAnim();
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

    //Wipe current sheet
    cleanMemory();
    sheet->updateSceneBounds();
    mSheetZoom->reset();
    msheetScene->setSceneRect(0,0,0,0);

    drawAnimation();

    sCurFilename = UNTITLED_IMAGE_STR;
    setModified(false);
    updateWindowTitle();

    //Clear undo/redo
    clearUndo();
    clearRedo();
    updateSelectedAnim();
    mAnimFrame = 0;
}

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    //TODO allow deleting currently selected animation(s)


    //Delete current selected frame(s)
    if(e->key() == Qt::Key_Delete && sheet && sheet->hasSelectedFrames())
    {
        deleteSelected();
    }
}

void MainWindow::on_saveFrameButton_clicked()
{
    if(!sheet || !sheet->size() || !mCurFrame)
        return;

    setIconImage(*mCurFrame);
    mIconExportWindow->show();
}

void MainWindow::on_fontColSelect_clicked()
{
    QColor selected = colorSelect.getColor(sheet->getFontColor(), this, "Select Font Color");
    if(selected.isValid())
        addUndoStep(new FontColorStep(this, sheet->getFontColor(), selected));
}

void MainWindow::on_frameBgColSelect_clicked()
{
    QColor selected = colorSelect.getColor(sheet->getFrameBgCol(), this, "Select Frame Background Color");
    if(selected.isValid())
        addUndoStep(new FrameBgColorStep(this, sheet->getFrameBgCol(), selected));
}

void MainWindow::on_sheetBgColSelect_clicked()
{
    QColor selected = colorSelect.getColor(sheet->getBgCol(), this, "Select Sheet Background Color");
    if(selected.isValid())
        addUndoStep(new SheetBgColorStep(this, sheet->getBgCol(), selected));
}

void MainWindow::on_frameBgTransparent_toggled(bool checked)
{
    if(bUIMutex)
        return;

    addUndoStep(new FrameBgTransparentStep(this, !checked, checked));
}

void MainWindow::on_sheetBgTransparent_toggled(bool checked)
{
    if(bUIMutex)
        return;

    addUndoStep(new SheetBgTransparentStep(this, !checked, checked));
}

void MainWindow::on_balanceAnimButton_clicked()
{
    if(!sheet || !sheet->size())
        return;

    Animation* anim = sheet->getAnimation(sheet->getCurSelected());
    if(!anim)
        return;

    QPoint curAnimSz = anim->getMaxFrameSize();
    setBalanceDefWH(curAnimSz.x(), curAnimSz.y());

    mBalanceWindow->show();
    centerParent(this, mBalanceWindow);
}

void MainWindow::balance(int w, int h, BalancePos::Pos vert, BalancePos::Pos horiz)
{
    if(!sheet || !sheet->size())
        return;

    Animation* anim = sheet->getAnimation(sheet->getCurSelected());
    if(!anim)
        return;

    anim->balance(QPoint(w,h), vert, horiz);
    sheet->refresh();

    drawAnimation();
    updateSelectedAnim();
    //genUndoState();
}

void MainWindow::undo()
{
    if(undoStack.size())
    {
        //Undo undoes a save state
        if(!bFileModified)
            setModified(true);

        //Save our redo point
        UndoStep* step = undoStack.pop();
        redoStack.push(step);

        //Undo
        bUIMutex = true;  //Don't bork on UI changes
        step->undo();
        bUIMutex = false;

        updateUndoRedoMenu();
    }
}

void MainWindow::redo()
{
    if(redoStack.size())
    {
        //Rndo undoes a save state
        if(!bFileModified)
            setModified(true);

        //Save this back on our undo list
        UndoStep* step = redoStack.pop();
        undoStack.push(step);

        //Load this state
        bUIMutex = true;  //Don't bork on UI changes
        step->redo();
        bUIMutex = false;

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

            //Reset GUI stuff!
            sheet->updateSceneBounds();
            mSheetZoom->reset();
            mSheetZoom->getView()->centerOn(sheet->getWidth()/2.0, sheet->getHeight()/2.0);

            QFileInfo fi(openFilename);
            sCurFilename = fi.fileName();
            setModified(false);
            updateWindowTitle();
            lastSaveStr = openFilename;
            clearUndo();
            clearRedo();
        }
    }
}

void MainWindow::saveToStream(QDataStream& s)
{
    int major = MAJOR_VERSION;
    int minor = MINOR_VERSION;
    int rev = REV_VERSION;
    s << major << minor << rev;  //Later we'll care about this if the save format changes again
    s << ui->minWidthCheckbox->isChecked(); //v1.2
    sheet->saveToStream(s);
}

void MainWindow::loadFromStream(QDataStream& s)
{
    bUIMutex = true;
    //Grab sheet frames
    int major = MAJOR_VERSION;
    int minor = MINOR_VERSION;
    int rev = REV_VERSION;
    s >> major >> minor >> rev;
    //Error out if file too new
    if(major > MAJOR_VERSION || (major == MAJOR_VERSION && minor > MINOR_VERSION) || (major == MAJOR_VERSION && minor == MINOR_VERSION && rev > REV_VERSION))
    {
        QMessageBox::warning(this, "File Load", "This sheet file was created with a newer version of Sprite Sheeter than you currently have. Please update your Sprite Sheeter version.");
        return;
    }
    //v 1.2: Load sheet width minimized checkbox
    bool bMinimizeSheet = true;
    if(major >= 1 && minor >= 2)
        s >> bMinimizeSheet;
    ui->minWidthCheckbox->setChecked(bMinimizeSheet);

    int numAnims = 0;
    s >> numAnims;
    for(int i = 0; i < numAnims; i++)
    {
        QVector<Frame*> imgList;
        int numFrames = 0;
        s >> numFrames;
        for(int j = 0; j < numFrames; j++)
        {
            bool selected = false;
            QImage* img = new QImage();
            QByteArray imgByteArray;
            s >> imgByteArray;
            QBuffer buffer(&imgByteArray);
            buffer.open(QIODevice::ReadOnly);
            if(major == 1 && minor < 2) //pre-v1.2 saved as TIFF
                img->load(&buffer, "TIFF");
            else
            {
                img->load(&buffer, "PNG");
                s >> selected;
            }
            Frame* f = new Frame(msheetScene, img, sheet->getFrameBgCol(), transparentBg, false);   //Will set framebgtransparent later
            if(selected)
                f->selectToggle();
            imgList.push_back(f);
        }

        //Create new animation
        Animation* animation = new Animation(transparentBg, msheetScene, this);
        animation->addImages(imgList, 0);
        sheet->addAnimation(animation);
        updateSelectedAnim();
    }

    //Grab anim names
    int numAnimNames = 0;
    s >> numAnimNames;
    if((unsigned int)numAnimNames > sheet->size())
        numAnimNames = sheet->size();
    for(int i = 0; i < numAnimNames; i++)
    {
        QString str;
        s >> str;
        sheet->getAnimation(i)->setName(str);
    }

    //Read other stuff
    QColor frameBgCol;
    QColor sheetBgCol;
    s >> sheetBgCol;
    s >> frameBgCol;
    QColor fontColor;
    if(major > 1 || (major == 1 && minor > 1))  //Version 1.2 introduced font color
        s >> fontColor;

    bool bSheetBg, bFrameBg;
    s >> bFrameBg >> bSheetBg;
    ui->frameBgTransparent->setChecked(bFrameBg);
    ui->sheetBgTransparent->setChecked(bSheetBg);
    ui->frameBgColSelect->setEnabled(!bFrameBg);
    ui->sheetBgColSelect->setEnabled(!bSheetBg);

    int xSpacing, ySpacing, sheetWidth;
    s >> xSpacing >> ySpacing >> sheetWidth;
    ui->xSpacingBox->setValue(xSpacing);
    ui->ySpacingBox->setValue(ySpacing);
    lastXSpacing = ui->xSpacingBox->value();
    lastYSpacing = ui->ySpacingBox->value();
    //Set sheet spacing now
    sheet->setXSpacing(ui->xSpacingBox->value());
    sheet->setYSpacing(ui->ySpacingBox->value());
    ui->sheetWidthBox->setValue(sheetWidth);
    lastSheetW = ui->sheetWidthBox->value();

    QString sFontStr;
    QFont sheetFont;
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

    //Init sheet values
    sheet->setBgCol(sheetBgCol);
    sheet->setFrameBgCol(frameBgCol);
    sheet->setBgTransparent(ui->sheetBgTransparent->isChecked());
    sheet->setFrameBgTransparent(ui->frameBgTransparent->isChecked());
    sheet->setFont(sheetFont);
    sheet->setFontColor(fontColor);
    sheet->setWidth(sheetWidth);
    if(bMinimizeSheet)
        minimizeSheetWidth();
    sheet->updateSceneBounds();

    //Fill in frame/sheet colors
    setColorButtonIcons();

    drawAnimation();
    updateSelectedAnim();
    bUIMutex = false;
}

void MainWindow::cleanMemory()
{
    //Wipe sheet
    sheet->clear();
}

void MainWindow::on_fontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, sheet->getFont(), this);
    if(ok)
        addUndoStep(new SheetFontStep(this, sheet->getFont(), font));
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

void MainWindow::addUndoStep(UndoStep* step)
{
    bUIMutex = true;    //Don't infinitely recurse if this step causes UI changes
    step->redo();   //Apply the step first
    bUIMutex = false;
    undoStack.push(step);

    //Clear redo list
    clearRedo();

    //Set the window title if this is the first the file has been modified
    if(!bFileModified)
    {
        setModified(true);
        updateWindowTitle();
    }

    updateUndoRedoMenu();
}

void MainWindow::clearUndo()
{
    while(undoStack.size())
    {
        UndoStep* st = undoStack.pop();
        delete st;
    }
    updateUndoRedoMenu();
}

void MainWindow::clearRedo()
{
    while(redoStack.size())
    {
        UndoStep* st = redoStack.pop();
        delete st;
    }
    updateUndoRedoMenu();
}

void MainWindow::updateUndoRedoMenu()
{
    ui->actionRedo->setEnabled(redoStack.size() > 0);
    ui->actionUndo->setEnabled(undoStack.size() > 0);
    ui->redoButton->setEnabled(redoStack.size() > 0);
    ui->undoButton->setEnabled(undoStack.size() > 0);
}

void MainWindow::on_xSpacingBox_editingFinished()
{
    if(bUIMutex)
        return;

    addUndoStep(new XSpacingStep(this, lastXSpacing, ui->xSpacingBox->value(), ui->sheetWidthBox->value()));

    lastXSpacing = ui->xSpacingBox->value();
}

void MainWindow::on_ySpacingBox_editingFinished()
{
    if(bUIMutex)
        return;

    addUndoStep(new YSpacingStep(this, lastYSpacing, ui->ySpacingBox->value()));
    lastYSpacing = ui->ySpacingBox->value();
}

void MainWindow::on_sheetWidthBox_editingFinished()
{
    if(bUIMutex)
        return;

    if(ui->minWidthCheckbox->isChecked())
        minimizeSheetWidth();
    sheet->updateSceneBounds();
    //genUndoState();
    lastSheetW = ui->sheetWidthBox->value();
}

void MainWindow::on_animationNameEditor_editingFinished()
{
    //genUndoState();
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
    //genUndoState();
}

//See http://sourceforge.net/p/freeimage/discussion/36111/thread/ea987d97/ for discussion of FreeImage gif saving...
void MainWindow::on_exportAnimButton_clicked()
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

        if(sheet->size())
            sheet->getAnimation(sheet->getCurSelected())->saveGIF(saveFilename, ui->animationSpeedSpinbox->value());
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
    checkMinWidth();
    drawAnimation();
    //genUndoState();

    FreeImage_CloseMultiBitmap(bmp);
    return true;
}

void MainWindow::on_reverseAnimButton_clicked()
{
    if(sheet->size())
    {
        Animation* anim = sheet->getAnimation(sheet->getCurSelected());
        if(anim)
        {
            anim->reverse();
            sheet->refresh();   //Tell sheet to recalculate positions

            drawAnimation();
            updateSelectedAnim();
            //genUndoState();
        }
    }

}

void MainWindow::on_removeDuplicateFramesButton_clicked()
{
    if(sheet && sheet->size())
    {
        Animation* anim = sheet->getAnimation(sheet->getCurSelected());
        if(anim)
        {
            if(anim->removeDuplicateFrames())
                sheet->refresh();

            mAnimFrame = 0;
            drawAnimation();
            updateSelectedAnim();

            //genUndoState();
        }
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
        batchRenderer->sheetFont = sheet->getFont();
        batchRenderer->maxSheetWidth = ui->sheetWidthBox->value();
        batchRenderer->offsetX = ui->xSpacingBox->value();
        batchRenderer->offsetY = ui->ySpacingBox->value();
        batchRenderer->animNameEnabled = ui->animNameEnabled->isChecked();
        batchRenderer->sheetBgTransparent = ui->sheetBgTransparent->isChecked();
        batchRenderer->sheetBgCol = sheet->getBgCol();
        batchRenderer->animHighlightCol = animHighlightCol;
        batchRenderer->frameBgTransparent = ui->frameBgTransparent->isChecked();
        batchRenderer->frameBgCol = sheet->getFrameBgCol();
        batchRenderer->fontColor = sheet->getFontColor();

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

    colIcon.fill(sheet->getBgCol());
    ui->sheetBgColSelect->setIcon(QIcon(colIcon));

    colIcon.fill(sheet->getFrameBgCol());
    ui->frameBgColSelect->setIcon(QIcon(colIcon));

    colIcon.fill(sheet->getFontColor());
    ui->fontColSelect->setIcon(QIcon(colIcon));
}

void MainWindow::on_minWidthCheckbox_toggled(bool checked)
{
    if(sheet && checked)
        minimizeSheetWidth();
    sheet->updateSceneBounds();
}

void MainWindow::minimizeSheetWidth()
{
    unsigned int width = sheet->getMinWidth();
    ui->sheetWidthBox->setValue(width); //Updates width of sheet automatically
    lastSheetW = width;
}

void MainWindow::deleteSelected()
{
    if(!sheet->size())
        return;

    sheet->deleteSelected();
    if(ui->minWidthCheckbox->isChecked())
        minimizeSheetWidth();
    curSelectedRect->setVisible(false); //In case we deleted a hovered frame
    //genUndoState();
    lastSelected = selected = NULL;
    curDragLine->setVisible(false); //If we're currently dragging, hide dragging line
    drawAnimation();
    updateSelectedAnim();
}

void MainWindow::setModified(bool b)
{
    bFileModified = b;
    ui->saveButton->setEnabled(b);
    ui->actionSave->setEnabled(b);
}

QImage* MainWindow::loadImageFI(QString filename)
{
    std::string sFilename = filename.toStdString();
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
    //pointer to the image, once loaded
    FIBITMAP *dib = NULL;
    //pointer to the image data
    BYTE* bits = NULL;
    //image width and height
    unsigned int width = 0, height = 0;

    //check the file signature and deduce its format
    fif = FreeImage_GetFileType(sFilename.c_str(), 0);
    //if still unknown, try to guess the file format from the file extension
    if(fif == FIF_UNKNOWN)
        fif = FreeImage_GetFIFFromFilename(sFilename.c_str());
    //if still unkown, return failure
    if(fif == FIF_UNKNOWN)
    {
        qDebug() << "Unknown image type for file " << filename << endl;
        return NULL;
    }

    //check that the plugin has reading capabilities and load the file
    if(FreeImage_FIFSupportsReading(fif))
        dib = FreeImage_Load(fif, sFilename.c_str());
    //if the image failed to load, return failure
    if(!dib)
    {
        qDebug() << "Error loading image " << filename << endl;
        return NULL;
    }
    //retrieve the image data

    //get the image width and height
    width = FreeImage_GetWidth(dib);
    height = FreeImage_GetHeight(dib);

    //Convert to 32 BPP if we need to
    if(FreeImage_GetBPP(dib) != 32)
    {
        FIBITMAP* frame32bit = FreeImage_ConvertTo32Bits(dib);
        FreeImage_Unload(dib);
        dib = frame32bit;
    }

    bits = FreeImage_GetBits(dib);
    if((bits == NULL) || (width == 0) || (height == 0))
    {
        qDebug() << "Unable to lock bits" << endl;
        return NULL;
    }

    QImage imgResult(bits, width, height, FreeImage_GetPitch(dib), QImage::Format_ARGB32);
    QImage* ret = new QImage(imgResult.mirrored());    //Copy the image cause Qt is dumb with image memory
    FreeImage_Unload(dib);
    return ret;
}

void MainWindow::checkMinWidth()
{
    unsigned int curW = ui->sheetWidthBox->value();
    unsigned int minW = sheet->getSmallestPossibleWidth();
    if(curW < minW)
    {
        ui->sheetWidthBox->setValue(minW);
        sheet->updateSceneBounds();
        lastSheetW = ui->sheetWidthBox->value();
    }
}

void MainWindow::updatePlayIcon()
{
    if(animUpdateTimer->isActive())
    {
        ui->animPlayButton->setIcon(QIcon("://images/pause"));
        ui->animPlayButton->setToolTip("Pause animation");
    }
    else
    {
        ui->animPlayButton->setIcon(QIcon("://images/media-play"));
        ui->animPlayButton->setToolTip("Play animation");
    }
}

void MainWindow::updateSelectedAnim()
{
    QString animationName;
    int curAnim = sheet->getCurSelected();
    if(curAnim >= 0 && curAnim < (int)sheet->size())
    {
        animationName = sheet->getAnimation(curAnim)->getName();
        Animation* anim = sheet->getAnimation(curAnim);
        if(anim)
        {
            curSelectedAnimRect->setVisible(true);
            int animHeight = anim->getCurHeight();
            int animStartY = anim->getPosY() + ui->ySpacingBox->value() / 2.0;
            if(curAnim == 0)
            {
                animStartY = 0;
                animHeight += ui->ySpacingBox->value() / 2.0;
            }
            if(curAnim == sheet->size() - 1)
                animHeight += ui->ySpacingBox->value() / 2.0;
            curSelectedAnimRect->setRect(anim->getPosX(), animStartY, ui->sheetWidthBox->value() - 1, animHeight);
        }
    }
    else
    {
        curSelectedAnimRect->setVisible(false);
    }
    ui->animationNameEditor->setText(animationName);
}

void MainWindow::on_newButton_clicked()
{
    newFile();
}

void MainWindow::on_openButton_clicked()
{
    loadSheet();
}

void MainWindow::on_saveButton_clicked()
{
    saveFile();
}

void MainWindow::on_cutButton_clicked()
{
    cut();
}

void MainWindow::on_copyButton_clicked()
{
    copy();
}

void MainWindow::on_pasteButton_clicked()
{
    paste();
}

void MainWindow::on_undoButton_clicked()
{
    undo();
}

void MainWindow::on_redoButton_clicked()
{
    redo();
}

void MainWindow::on_removeAnimButton_clicked()  //Delete button
{
    deleteSelected();
}

void MainWindow::cut()
{
    //TODO
}

void MainWindow::copy()
{
    //TODO
}

void MainWindow::paste()
{
    //TODO
}
