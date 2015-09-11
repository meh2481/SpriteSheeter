#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Graphics_view_zoom.h"
#include "sheeteditorview.h"
#include <QFileDialog>
#include <QDesktopWidget>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QFontDialog>
#include <QFontMetrics>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mImportWindow = new importDialog(this);
    mBalanceWindow = new balanceSheet(this);

    //Connect all our signals & slots up
    QObject::connect(mImportWindow, SIGNAL(importOK(int, int, bool, bool)), this, SLOT(importNext(int, int, bool, bool)));
    QObject::connect(mImportWindow, SIGNAL(importAll(int, int, bool, bool)), this, SLOT(importAll(int, int, bool, bool)));
    QObject::connect(this, SIGNAL(setImportImg(QString)), mImportWindow, SLOT(setPreviewImage(QString)));
    QObject::connect(ui->sheetPreview, SIGNAL(mouseMoved(int,int)), this, SLOT(mouseCursorPos(int, int)));
    QObject::connect(ui->sheetPreview, SIGNAL(mousePressed(int,int)), this, SLOT(mouseDown(int, int)));
    QObject::connect(ui->sheetPreview, SIGNAL(mouseReleased(int,int)), this, SLOT(mouseUp(int, int)));
    QObject::connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveFile()));
    QObject::connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(saveFileAs()));
    QObject::connect(ui->actionImport_WIP_Sheet, SIGNAL(triggered(bool)), this, SLOT(loadSheet()));
    QObject::connect(ui->actionUndo, SIGNAL(triggered(bool)), this, SLOT(undo()));
    QObject::connect(ui->actionRedo, SIGNAL(triggered(bool)), this, SLOT(redo()));
    QObject::connect(ui->actionEnableShortcuts, SIGNAL(toggled(bool)), this, SLOT(enableShortcuts(bool)));
    QObject::connect(ui->sheetPreview, SIGNAL(droppedFiles(QStringList)), this, SLOT(addImages(QStringList)));
    QObject::connect(ui->sheetPreview, SIGNAL(droppedFolders(QStringList)), this, SLOT(addFolders(QStringList)));
    QObject::connect(mBalanceWindow, SIGNAL(balance(int,int,balanceSheet::Pos,balanceSheet::Pos)), this, SLOT(balance(int,int,balanceSheet::Pos,balanceSheet::Pos)));
    QObject::connect(this, SIGNAL(setBalanceDefWH(int,int)), mBalanceWindow, SLOT(defaultWH(int,int)));

    animItem = NULL;
    animScene = NULL;
    sheetItem = NULL;
    msheetScene = NULL;
    mCurSheet = NULL;
    transparentBg = new QImage("://bg");
    mCurAnim = mSheetFrames.begin();
    mCurAnimName = mAnimNames.begin();
    bShortcuts = true;

    bDraggingSheetW = false;
    bFileModified = false;
    sCurFilename = UNTITLED_IMAGE_STR;
    //TODO Store initial undo state

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

    Graphics_view_zoom* z = new Graphics_view_zoom(ui->sheetPreview);
    z->set_modifiers(Qt::NoModifier);

    z = new Graphics_view_zoom(ui->animationPreview);
    z->set_modifiers(Qt::NoModifier);

    sheetBgCol = QColor(0, 128, 128, 255);
    frameBgCol = QColor(0, 255, 0, 255);

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
        fileFilters << "*.png" << "*.bmp" << "*.gif" << "*.pbm" << "*.pgm" << "*.ppm" << "*.tif" << "*.tiff" << "*.xbm" << "*.xpm" << "*.tga";
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
    mOpenFiles = QFileDialog::getOpenFileNames(this, "Import Image Sequence", lastOpenDir, "All Files (*.*)");
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
        if(sName->length())
            iSizeY += textHeight;
        sName++;

        if(iCurSizeX > iSizeX)
            iSizeX = iCurSizeX;
    }

    if(mCurSheet)
        delete mCurSheet;

    if(bHighlight)
        iSizeX += DRAG_HANDLE_SIZE;

    //Create image of the proper size and fill it with a good bg color
    mCurSheet = new QImage(iSizeX, iSizeY, QImage::Format_ARGB32);
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
        r.setRect(0,curY-offsetY,iSizeX,0);
        int ySize = 0;

        //Highlight our current anim red
        if(bHighlight && sName == mCurAnimName)
        {
            if(sName->length())
                painter.fillRect(0, curY-offsetY, mCurSheet->width(), hiliteH + textHeight + offsetY*2, QColor(128,0,0,255));
            else
                painter.fillRect(0, curY-offsetY, mCurSheet->width(), hiliteH + offsetY*2, QColor(128,0,0,255));
        }

        //Draw label for animation
        if(sName->length())
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
                painter.fillRect(QRect(curX, curY, img->width(), img->height()), QBrush(frameBgCol));
            }

            painter.drawImage(curX, curY, *img);

            //If we're highlighting this image, draw blue overtop
            if(bHighlight && mCurSelected == img)
            {
                painter.fillRect(QRect(curX, curY, img->width(), img->height()), QBrush(QColor(0,0,255,100)));
                painter.setCompositionMode(QPainter::CompositionMode_Source);
            }

            if(img->height() > ySize)
                ySize = img->height();
            curX += img->width() + offsetX;
        }
        curY += offsetY + ySize;
        curX = offsetX;
        r.setBottom(curY-offsetY);
        mAnimRects.push_back(r);
    }

    //Draw drag handle on right side
    if(bHighlight)
        painter.fillRect(iSizeX - DRAG_HANDLE_SIZE, 0, DRAG_HANDLE_SIZE, mCurSheet->height(), QColor(0,230,230,255));

    painter.end();


    //Update the GUI to show this image
    if(sheetItem == NULL)
    {
        sheetItem = new QGraphicsPixmapItem(QPixmap::fromImage(*mCurSheet));
        msheetScene->addItem(sheetItem);
    }
    else
        sheetItem->setPixmap(QPixmap::fromImage(*mCurSheet));

    //Set the new rect of the scene
    msheetScene->setSceneRect(-100, -100, mCurSheet->width()+200, mCurSheet->height()+200);

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

    //TODO
    if(mCurSheet)
    {
        //Update cursor if need be
        if(x <= mCurSheet->width() &&
           x >= mCurSheet->width() - DRAG_HANDLE_SIZE &&
           y <= mCurSheet->height() &&
           y >= 0)
        {
             ui->sheetPreview->setCursor(Qt::SizeHorCursor);
        }
        else if(!bDraggingSheetW)
            ui->sheetPreview->setCursor(Qt::ArrowCursor);

        //See if we're resizing the sheet
        if(bDraggingSheetW)
        {
            ui->sheetWidthBox->setValue(mStartSheetW - (xStartDragSheetW - x));
            drawSheet();
        }

    }
    statusBar()->showMessage(QString::number(x) + ", " + QString::number(y));

    QList<QImage>::iterator mPrevSelected = mCurSelected;

    if(mCurAnim != mSheetFrames.end())
        mCurSelected = mCurAnim->end();

    int maxSheetWidth = ui->sheetWidthBox->value();
    int offsetX = ui->xSpacingBox->value();
    int offsetY = ui->ySpacingBox->value();
    int curX = offsetX;
    int curY = offsetY;
    QList<QString>::iterator sName = mAnimNames.begin();
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

            //Check and see if we're overlapping this portion of the image
            //painter.fillRect(curX, curY, img.width(), img.height(), Qt::transparent);
            if(x >= curX && x < curX + img->width() &&
               y >= curY && y < curY + img->height())
            {
                mCurSelected = img;
                mCurSelectedInAnim = ql;
            }

            if(img->height() > ySize)
                ySize = img->height();
            curX += img->width() + offsetX;
        }
        curY += offsetY + ySize;
        curX = offsetX;
    }

    if(mPrevSelected != mCurSelected)
        drawSheet();

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
    }
    Q_UNUSED(y);    //TODO drag anim frames around...?
}

void MainWindow::closeEvent(QCloseEvent *event)
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
    settings.setValue("lastSaveStr", lastSaveStr);
    settings.setValue("lastIconStr", lastIconStr);
    settings.setValue("lastOpenDir", lastOpenDir);
    settings.setValue("lastImportExportStr", lastImportExportStr);
    settings.setValue("sheetFont", sheetFont.toString());
    settings.setValue("shortcuts", bShortcuts);
    //settings.setValue("", );
    QMainWindow::closeEvent(event);
}

void MainWindow::readSettings()
{
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
}

void MainWindow::on_sheetWidthBox_valueChanged(int arg1)
{
    //drawSheet();
    Q_UNUSED(arg1);
}

void MainWindow::newFile()
{
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
}

void MainWindow::saveFile()
{
    on_saveSheetButton_clicked();
}

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    //Deleting current selected frame
    if(e->key() == Qt::Key_Delete && mSheetFrames.size() && mCurAnim != mSheetFrames.end() && mCurSelected != mCurAnim->end())
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
                }

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
                }

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

    /*
    else if(e->key() == Qt::Key_Left)
    {
        on_animPrevFrameButton_clicked();
    }
    else if(e->key() == Qt::Key_Right)
    {
        on_animNextFrameButton_clicked();
    }*/
}

void MainWindow::on_saveFrameButton_clicked()
{
    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size() || mCurFrame == mCurAnim->end())
        return;

    QString sSel = "PNG Image (*.png)";
    QString saveFilename = QFileDialog::getSaveFileName(this,
                                                        tr("Save TSR Icon"),
                                                        lastIconStr,
                                                        tr("PNG Image (*.png);;Windows Bitmap (*.bmp);;TIFF Image(*.tiff)"),
                                                        &sSel);

    if(saveFilename.length())
    {
        QImage icon(*mCurFrame);

        if(icon.width()* 2 <= ICON_WIDTH && icon.height() * 2 <= ICON_HEIGHT)   //Scale image up
            icon = icon.scaledToWidth(icon.width()*2, Qt::FastTransformation);
        else if(icon.width() > ICON_WIDTH || icon.height() > ICON_HEIGHT)       //Scale image down
        {
            //Scale down X or Y as appropriate to fit the whole image
            float wFac = (float)ICON_WIDTH / (float)icon.width();
            if(wFac * icon.height() > ICON_HEIGHT)
                icon = icon.scaledToHeight(ICON_HEIGHT, Qt::SmoothTransformation);
            else
                icon = icon.scaledToWidth(ICON_WIDTH, Qt::SmoothTransformation);
        }

        //icon = icon.copy(0, 0, ICON_WIDTH, ICON_HEIGHT);
        QImage saveIcon(ICON_WIDTH, ICON_HEIGHT, QImage::Format_ARGB32);
        saveIcon.fill(QColor(0,0,0,0));
        QPainter paint(&saveIcon);
        paint.drawImage((ICON_WIDTH - icon.width())/2, (ICON_HEIGHT - icon.height())/2, icon);
        paint.end();

        saveIcon.save(saveFilename);

        lastIconStr = saveFilename;
    }
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
    ui->frameBgColSelect->setEnabled(!checked);
    drawSheet();
    drawAnimation();
    genUndoState();
}

void MainWindow::on_SheetBgTransparent_toggled(bool checked)
{
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

void MainWindow::balance(int w, int h, balanceSheet::Pos vert, balanceSheet::Pos horiz)
{
    if(mCurAnim == mSheetFrames.end() || !mCurAnim->size() || mCurFrame == mCurAnim->end())
        return;

    QMutableListIterator<QImage> it(*mCurAnim);
    while(it.hasNext())
    {
        QImage img = it.next();

        //Use vert/horiz
        int xPos, yPos;
        if(vert == balanceSheet::Up)
            yPos = 0;
        else if(vert == balanceSheet::Mid)
            yPos = (h/2)-(img.height()/2);
        else
            yPos = h - img.height();

        if(horiz == balanceSheet::Left)
            xPos = 0;
        else if(horiz == balanceSheet::Mid)
            xPos = (w/2)-(img.width()/2);
        else
            xPos = w - img.width();

        QImage final(w, h, QImage::Format_ARGB32);
        final.fill(QColor(0,0,0,0));
        QPainter painter(&final);
        painter.drawImage(xPos, yPos, img);
        painter.end();
        it.setValue(final);
     }

    drawSheet();
    genUndoState();
}


void MainWindow::undo()
{
    //TODO
}

void MainWindow::redo()
{
    //TODO
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
        QFile f(filename);
        if(f.open(QIODevice::WriteOnly))
        {
            QDataStream s(&f);

            //Save sheet frames
            s << mSheetFrames.size();
            foreach(QList<QImage> imgList, mSheetFrames)
            {
                 s << imgList.size();
                 foreach(QImage img, imgList)
                     s << img;
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

            //TODO Save current anim/anim frame
        }
    }
}

void MainWindow::loadSheet()
{
    QString openFilename = QFileDialog::getOpenFileName(this, "Import WIP Sheet", lastImportExportStr, "Sprite Sheets (*.sheet)");

    if(openFilename.length())
    {
        lastImportExportStr = openFilename;
        QFile f(openFilename);
        if(f.open(QIODevice::ReadOnly))
        {
            QDataStream s(&f);

            //Clean up memory
            cleanMemory();

            //Grab sheet frames
            int numAnims = 0;
            s >> numAnims;
            for(int i = 0; i < numAnims; i++)
            {
                QList<QImage> imgList;
                int numFrames = 0;
                s >> numFrames;
                for(int j = 0; j < numFrames; j++)
                {
                    QImage img;
                    s >> img;
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
            bool bSheetBg, bFrameBg;
            s >> bFrameBg >> bSheetBg;
            ui->FrameBgTransparent->setChecked(bFrameBg);
            ui->SheetBgTransparent->setChecked(bSheetBg);
            int xSpacing, ySpacing, sheetWidth;
            s >> xSpacing >> ySpacing >> sheetWidth;
            ui->xSpacingBox->setValue(xSpacing);
            ui->ySpacingBox->setValue(ySpacing);
            ui->sheetWidthBox->setValue(sheetWidth);
            QString sFontStr;
            s >> sFontStr;
            if(sFontStr.size())
                sheetFont.fromString(sFontStr);

            //TODO Load current anim/anim frame

            //Set stuff in the GUI correctly
            mCurAnim = mSheetFrames.begin();
            mCurAnimName = mAnimNames.begin();
            drawSheet();
            if(mCurAnimName != mAnimNames.end())
                ui->animationNameEditor->setText(*mCurAnimName);
            if(mCurAnim != mSheetFrames.end())
                mCurFrame = mCurAnim->begin();
            drawAnimation();

            QFileInfo fi(openFilename);
            sCurFilename = fi.fileName();
            bFileModified = false;
            fixWindowTitle();
            lastSaveStr = openFilename;
            //TODO Store file orig state
        }
    }
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
        sWindowTitle << "." << REV_VERSION;
    setWindowTitle(sWindowStr);
}

void MainWindow::genUndoState()
{
    //Set the window title if this is the first the file has been modified
    if(!bFileModified)
    {
        bFileModified = true;
        fixWindowTitle();
    }

    //TODO Gen undo point

    //TODO Clear redo list

    //TODO Enable undo menu button if it's disabled

    //TODO Disable redo menu button
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














































