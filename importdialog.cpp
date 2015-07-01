#include "importdialog.h"
#include "ui_importdialog.h"

importDialog::importDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::importDialog)
{
    ui->setupUi(this);
}

importDialog::~importDialog()
{
    delete ui;
}

void importDialog::on_okButton_clicked()
{
    this->hide();
    //TODO
}

void importDialog::on_allButton_clicked()
{
    this->hide();
    //TODO
}
