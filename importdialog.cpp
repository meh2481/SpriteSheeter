#include "importdialog.h"
#include "ui_importdialog.h"

importDialog::importDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::importDialog)
{
    ui->setupUi(this);

    scene = NULL;
    //view = NULL;
    item = NULL;
}

importDialog::~importDialog()
{
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
    importOK(ui->xFramesBox->value(), ui->yFramesBox->value());
}

void importDialog::on_allButton_clicked()
{
    this->hide();
    importAll(ui->xFramesBox->value(), ui->yFramesBox->value());
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
















