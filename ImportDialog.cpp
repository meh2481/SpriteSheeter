#include "ImportDialog.h"
#include "ui_ImportDialog.h"
#include <QSettings>

ImportDialog::ImportDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::importDialog)
{
    ui->setupUi(this);

    scene = NULL;
    item = NULL;
    restoreSettings();
}

ImportDialog::~ImportDialog()
{
    saveSettings();
    if(item)
        delete item;
    if(scene)
        delete scene;
    delete ui;
}

void ImportDialog::setPreviewImage(QString sImg)
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

void ImportDialog::on_okButton_clicked()
{
    this->hide();
    importOK(ui->xFramesBox->value(), ui->yFramesBox->value(), ui->vertFirst->isChecked(), ui->checkSplitAnimations->isChecked());
}

void ImportDialog::on_allButton_clicked()
{
    this->hide();
    importAll(ui->xFramesBox->value(), ui->yFramesBox->value(), ui->vertFirst->isChecked(), ui->checkSplitAnimations->isChecked());
}

void ImportDialog::showEvent(QShowEvent *)
{
    ui->imagePreview->fitInView(scene->sceneRect(),Qt::KeepAspectRatio);
}

void ImportDialog::resizeEvent(QResizeEvent* event)
{
   QDialog::resizeEvent(event);

   ui->imagePreview->fitInView(scene->sceneRect(),Qt::KeepAspectRatio);
}

void ImportDialog::saveSettings()
{
    QSettings settings("DaxarDev", "SpriteSheeterImport");
    //Save other GUI settings
    settings.setValue("xFrames", ui->xFramesBox->value());
    settings.setValue("yFrames", ui->yFramesBox->value());
    settings.setValue("horizFirst", ui->horizFirst->isChecked());
    settings.setValue("vertFirst", ui->vertFirst->isChecked());
    settings.setValue("splitAnimations", ui->checkSplitAnimations->isChecked());
    //settings.setValue("", );
}

void ImportDialog::restoreSettings()
{
    QSettings settings("DaxarDev", "SpriteSheeterImport");
    if(settings.value("xFrames", -1).toInt() == -1)    //No settings are here
        return;
    ui->xFramesBox->setValue(settings.value("xFrames").toInt());
    ui->yFramesBox->setValue(settings.value("yFrames").toInt());
    ui->horizFirst->setChecked(settings.value("horizFirst").toBool());
    ui->vertFirst->setChecked(settings.value("vertFirst").toBool());
    ui->checkSplitAnimations->setChecked(settings.value("splitAnimations").toBool());
}

void ImportDialog::on_cancelButton_clicked()
{
    this->hide();
}




























