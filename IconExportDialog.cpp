#include "IconExportDialog.h"
#include "ui_IconExportDialog.h"
#include <QSettings>
#include <QFileDialog>
#include <QMouseEvent>
#include <QDebug>

IconExportDialog::IconExportDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::iconExport)
{
    ui->setupUi(this);

    ui->iconPreview->viewport()->installEventFilter(this);

    scene = NULL;
    item = NULL;

    iconScale = 1.0f;

    bMouseDown = false;

    transparentBg = new QImage("://bg");
    loadSettings();
}

IconExportDialog::~IconExportDialog()
{
    saveSettings();
    if(transparentBg)
        delete transparentBg;
    if(item)
        delete item;
    if(scene)
        delete scene;
    delete ui;
}

void IconExportDialog::setImage(QImage img)
{
    iconImg = img;
    drawPreview();
}

void IconExportDialog::on_fitXButton_clicked()
{
    iconScale = ((float)ICON_WIDTH)/((float)iconImg.width());
    if(iconScale > 1.0f)
    {
        iconScale = (int)iconScale;
        if(iconScale > 3)
            iconScale = 3;
        ui->horizontalSlider->setValue(iconScale-1);
    }
    on_centerButton_clicked();
}

void IconExportDialog::on_fitYButton_clicked()
{
    iconScale = ((float)ICON_HEIGHT)/((float)iconImg.height());
    if(iconScale > 1.0f)
    {
        iconScale = (int)iconScale;
        if(iconScale > 3)
            iconScale = 3;
        ui->horizontalSlider->setValue(iconScale-1);
    }
    on_centerButton_clicked();
}

void IconExportDialog::on_resetButton_clicked()
{
    ui->offsetXBox->setValue(0);
    ui->offsetYBox->setValue(0);
    ui->horizontalSlider->setValue(0);
    iconScale = 1.0f;
    updateScaleText();
    drawPreview();
}

void IconExportDialog::updateScaleText()
{
    QString sScaleText = QString::number(iconScale, 'f', 2);
    sScaleText += "x";
    ui->scaleFacBox->setText(sScaleText);
}

void IconExportDialog::on_horizontalSlider_sliderMoved(int position)
{
    Q_UNUSED(position);
}

void IconExportDialog::on_offsetXBox_valueChanged(int arg1)
{
    drawPreview();
    Q_UNUSED(arg1);
}

void IconExportDialog::on_offsetYBox_valueChanged(int arg1)
{
    drawPreview();
    Q_UNUSED(arg1);
}

void IconExportDialog::on_centerButton_clicked()
{
    int x = iconImg.width()*iconScale;
    int y = iconImg.height()*iconScale;
    ui->offsetXBox->setValue((ICON_WIDTH/2)-(x/2));
    ui->offsetYBox->setValue((ICON_HEIGHT/2)-(y/2));
    drawPreview();
}

void IconExportDialog::on_saveIconBtn_clicked()
{
    QString sSel = "PNG Image (*.png)";
    if(lastIconStr.contains(".bmp", Qt::CaseInsensitive))
        sSel = "Windows Bitmap (*.bmp)";
    else if(lastIconStr.contains(".tiff", Qt::CaseInsensitive))
        sSel = "TIFF Image (*.tiff)";

    QString saveFilename = QFileDialog::getSaveFileName(this,
                                                        tr("Save TSR Icon"),
                                                        lastIconStr,
                                                        tr("PNG Image (*.png);;Windows Bitmap (*.bmp);;TIFF Image (*.tiff)"),
                                                        &sSel);

    if(saveFilename.length())
    {
        //Create image we'll save (almost identical to drawing code yaay duplicated code I should split into a function but I don't care)
        QImage icon(ICON_WIDTH, ICON_HEIGHT, QImage::Format_ARGB32);
        icon.fill(QColor(0,0,0,0)); //Fill transparent
        QPainter painter(&icon);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        Qt::TransformationMode mode = Qt::SmoothTransformation;
        if(iconScale > 1.0f)
            mode = Qt::FastTransformation;
        QImage imgScaled = iconImg.scaled(iconImg.width()*iconScale, iconImg.height()*iconScale, Qt::IgnoreAspectRatio, mode);
        painter.drawImage(ui->offsetXBox->value(), ui->offsetYBox->value(), imgScaled);
        painter.end();

        //Save the icon
        icon.save(saveFilename);
        lastIconStr = saveFilename;

        this->hide();
    }
}

void IconExportDialog::on_cancelBtn_clicked()
{
    this->hide();
}

void IconExportDialog::drawPreview()
{
    if(!iconImg.isNull())
    {
        //Create image we'll draw
        QImage image(ICON_WIDTH, ICON_HEIGHT, QImage::Format_ARGB32);
        QPainter painter(&image);
        painter.setCompositionMode(QPainter::CompositionMode_Source);

        //Fill image bg transparent
        QBrush bgTexBrush(*transparentBg);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.fillRect(0, 0, ICON_WIDTH, ICON_HEIGHT, bgTexBrush);

        //Scale image
        Qt::TransformationMode mode = Qt::SmoothTransformation;
        if(iconScale > 1.0f)
            mode = Qt::FastTransformation;
        QImage imgScaled = iconImg.scaled(iconImg.width()*iconScale, iconImg.height()*iconScale, Qt::IgnoreAspectRatio, mode);
        painter.drawImage(ui->offsetXBox->value(), ui->offsetYBox->value(), imgScaled);
        painter.end();

        if(!image.isNull())
        {
            if(scene == NULL)
            {
                scene = new QGraphicsScene();
                ui->iconPreview->setScene(scene);
            }
            if(item == NULL)
            {
                item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
                scene->addItem(item);
            }
            else
                item->setPixmap(QPixmap::fromImage(image));

            scene->setSceneRect(0, 0, image.width(), image.height());

            ui->iconPreview->show();
        }
    }
}

void IconExportDialog::on_horizontalSlider_valueChanged(int value)
{
    if(value >= 0)
        iconScale = value + 1;
    else
        iconScale = (30.0f - (float)-value) / 30.0f;

    updateScaleText();

    drawPreview();
}

void IconExportDialog::saveSettings()
{
    QSettings settings("DaxarDev", "SpriteSheeterIcon");
    settings.setValue("offsetXBox", ui->offsetXBox->value());
    settings.setValue("offsetYBox", ui->offsetYBox->value());
    settings.setValue("horizontalSlider", ui->horizontalSlider->value());
    settings.setValue("lastIconStr", lastIconStr);
}

void IconExportDialog::loadSettings()
{
    QSettings settings("DaxarDev", "SpriteSheeterIcon");
    if(settings.value("offsetXBox", -1).toInt() == -1)    //No settings are here
        return;
    ui->offsetXBox->setValue(settings.value("offsetXBox").toInt());
    ui->offsetYBox->setValue(settings.value("offsetYBox").toInt());
    ui->horizontalSlider->setValue(settings.value("horizontalSlider").toInt());
    lastIconStr = settings.value("lastIconStr").toString();
}

bool IconExportDialog::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == ui->iconPreview->viewport())
    {
        if(event->type() == QEvent::MouseMove)
        {
            if(bMouseDown)
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                ui->offsetXBox->setValue(ui->offsetXBox->value() + mouseEvent->pos().x() - lastx);
                ui->offsetYBox->setValue(ui->offsetYBox->value() + mouseEvent->pos().y() - lasty);
                lastx = mouseEvent->pos().x();
                lasty = mouseEvent->pos().y();
            }
        }
        else if(event->type() == QEvent::MouseButtonPress)
        {
            bMouseDown = true;
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            lastx = mouseEvent->pos().x();
            lasty = mouseEvent->pos().y();
        }
    }
    if(event->type() == QEvent::MouseButtonRelease)
        bMouseDown = false;

    return false;
}






















