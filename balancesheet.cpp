#include "balancesheet.h"
#include "ui_balancesheet.h"
#include <QSettings>

balanceSheet::balanceSheet(QWidget *parent) : QDialog(parent),
    ui(new Ui::balanceSheet)
{
    ui->setupUi(this);
    vertPos = horizPos = Mid;
}

balanceSheet::~balanceSheet()
{
    delete ui;
}





























void balanceSheet::on_okButton_clicked()
{
    this->hide();
    //TODO
}

void balanceSheet::on_cancelButton_clicked()
{
    this->hide();
}





void balanceSheet::on_pos_ul_clicked()
{
    vertPos = Up;
    horizPos = Left;
}

void balanceSheet::on_pos_um_clicked()
{
    vertPos = Up;
    horizPos = Mid;
}

void balanceSheet::on_pos_ur_clicked()
{
    vertPos = Up;
    horizPos = Right;
}

void balanceSheet::on_pos_ml_clicked()
{
    vertPos = Mid;
    horizPos = Left;
}

void balanceSheet::on_pos_mm_clicked()
{
    vertPos = Mid;
    horizPos = Mid;
}

void balanceSheet::on_pos_mr_clicked()
{
    vertPos = Mid;
    horizPos = Right;
}

void balanceSheet::on_pos_bl_clicked()
{
    vertPos = Down;
    horizPos = Left;
}

void balanceSheet::on_pos_bm_clicked()
{
    vertPos = Down;
    horizPos = Mid;
}

void balanceSheet::on_pos_br_clicked()
{
    vertPos = Down;
    horizPos = Right;
}
