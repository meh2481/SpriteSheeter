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


void balanceSheet::clearIcons()
{
    ui->pos_ul->setIcon(QIcon("://blank"));
    ui->pos_ur->setIcon(QIcon("://blank"));
    ui->pos_um->setIcon(QIcon("://blank"));
    ui->pos_ml->setIcon(QIcon("://blank"));
    ui->pos_mr->setIcon(QIcon("://blank"));
    ui->pos_mm->setIcon(QIcon("://blank"));
    ui->pos_bl->setIcon(QIcon("://blank"));
    ui->pos_br->setIcon(QIcon("://blank"));
    ui->pos_bm->setIcon(QIcon("://blank"));
}


void balanceSheet::on_pos_ul_clicked()
{
    vertPos = Up;
    horizPos = Left;
    clearIcons();
    ui->pos_ul->setIcon(QIcon("://stop"));
}

void balanceSheet::on_pos_um_clicked()
{
    vertPos = Up;
    horizPos = Mid;
    clearIcons();
    ui->pos_um->setIcon(QIcon("://stop"));
}

void balanceSheet::on_pos_ur_clicked()
{
    vertPos = Up;
    horizPos = Right;
    clearIcons();
    ui->pos_ur->setIcon(QIcon("://stop"));
}

void balanceSheet::on_pos_ml_clicked()
{
    vertPos = Mid;
    horizPos = Left;
    clearIcons();
    ui->pos_ml->setIcon(QIcon("://stop"));
}

void balanceSheet::on_pos_mm_clicked()
{
    vertPos = Mid;
    horizPos = Mid;
    clearIcons();
    ui->pos_mm->setIcon(QIcon("://stop"));
}

void balanceSheet::on_pos_mr_clicked()
{
    vertPos = Mid;
    horizPos = Right;
    clearIcons();
    ui->pos_mr->setIcon(QIcon("://stop"));
}

void balanceSheet::on_pos_bl_clicked()
{
    vertPos = Down;
    horizPos = Left;
    clearIcons();
    ui->pos_bl->setIcon(QIcon("://stop"));
}

void balanceSheet::on_pos_bm_clicked()
{
    vertPos = Down;
    horizPos = Mid;
    clearIcons();
    ui->pos_bm->setIcon(QIcon("://stop"));
}

void balanceSheet::on_pos_br_clicked()
{
    vertPos = Down;
    horizPos = Right;
    clearIcons();
    ui->pos_br->setIcon(QIcon("://stop"));
}




























