#include "importdialog.h"
#include "ui_importdialog.h"
#include <QSettings>

importDialog::importDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::importDialog)
{
    ui->setupUi(this);

    scene = NULL;
    //view = NULL;
    item = NULL;
    restoreSettings();
}

importDialog::~importDialog()
{
    saveSettings();
    if(item)
        delete item;
    if(scene)
        delete scene;
    delete ui;
}

void importDialog::setPreviewImage(QString sImg)
{
    if(!sImg.isEmpty())
    {
        QImage image(sImg);
        if(!image.isNull())
        {
            if(scene == NULL)
            {
                scene = new QGraphicsScene();
                ui->imagePreview->setScene(scene);
            }
            if(item == NULL)
            {
                item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
                scene->addItem(item);
            }
            else
                item->setPixmap(QPixmap::fromImage(image));

            scene->setSceneRect(0, 0, image.width(), image.height());

            ui->imagePreview->show();
        }
    }
}

void importDialog::on_okButton_clicked()
{
    this->hide();
    importOK(ui->xFramesBox->value(), ui->yFramesBox->value(), ui->vertFirst->isChecked(), ui->checkSplitAnimations->isChecked());
}

void importDialog::on_allButton_clicked()
{
    this->hide();
    importAll(ui->xFramesBox->value(), ui->yFramesBox->value(), ui->vertFirst->isChecked(), ui->checkSplitAnimations->isChecked());
}

void importDialog::showEvent(QShowEvent *)
{
    ui->imagePreview->fitInView(scene->sceneRect(),Qt::KeepAspectRatio);
}

void importDialog::resizeEvent(QResizeEvent* event)
{
   QDialog::resizeEvent(event);

   ui->imagePreview->fitInView(scene->sceneRect(),Qt::KeepAspectRatio);
}

void importDialog::saveSettings()
{
    QSettings settings("DaxarDev", "SpriteSheeterImport");
    //Save window geometry
    //settings.setValue("geometry", saveGeometry());
    //settings.setValue("windowState", saveState());
    //Save other GUI settings
    settings.setValue("xFrames", ui->xFramesBox->value());
    settings.setValue("yFrames", ui->yFramesBox->value());
    settings.setValue("horizFirst", ui->horizFirst->isChecked());
    settings.setValue("vertFirst", ui->vertFirst->isChecked());
    settings.setValue("splitAnimations", ui->checkSplitAnimations->isChecked());
    //settings.setValue("", );
}

void importDialog::restoreSettings()
{
    QSettings settings("DaxarDev", "SpriteSheeterImport");
    //restoreGeometry(settings.value("geometry").toByteArray());
    //restoreState(settings.value("windowState").toByteArray());
    if(settings.value("xFrames", -1).toInt() == -1)    //No settings are here
        return;
    ui->xFramesBox->setValue(settings.value("xFrames").toInt());
    ui->yFramesBox->setValue(settings.value("yFrames").toInt());
    ui->horizFirst->setChecked(settings.value("horizFirst").toBool());
    ui->vertFirst->setChecked(settings.value("vertFirst").toBool());
    ui->checkSplitAnimations->setChecked(settings.value("splitAnimations").toBool());
}



























