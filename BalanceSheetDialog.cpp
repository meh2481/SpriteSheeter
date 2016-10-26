#include "BalanceSheetDialog.h"
#include "ui_BalanceSheetDialog.h"
#include <QSettings>

BalanceSheetDialog::BalanceSheetDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::balanceSheet)
{
    ui->setupUi(this);
    vertPos = horizPos = Mid;
}

BalanceSheetDialog::~BalanceSheetDialog()
{
    delete ui;
}

void BalanceSheetDialog::on_okButton_clicked()
{
    this->hide();
    balance(ui->spriteWidth->value(), ui->spriteHeight->value(), vertPos, horizPos);
}

void BalanceSheetDialog::on_cancelButton_clicked()
{
    this->hide();
}

void BalanceSheetDialog::defaultWH(int w, int h)
{
    ui->spriteWidth->setValue(w);
    ui->spriteHeight->setValue(h);
}

void BalanceSheetDialog::clearIcons()
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


void BalanceSheetDialog::on_pos_ul_clicked()
{
    vertPos = Up;
    horizPos = Left;
    clearIcons();
    ui->pos_ul->setIcon(QIcon("://stop"));
}

void BalanceSheetDialog::on_pos_um_clicked()
{
    vertPos = Up;
    horizPos = Mid;
    clearIcons();
    ui->pos_um->setIcon(QIcon("://stop"));
}

void BalanceSheetDialog::on_pos_ur_clicked()
{
    vertPos = Up;
    horizPos = Right;
    clearIcons();
    ui->pos_ur->setIcon(QIcon("://stop"));
}

void BalanceSheetDialog::on_pos_ml_clicked()
{
    vertPos = Mid;
    horizPos = Left;
    clearIcons();
    ui->pos_ml->setIcon(QIcon("://stop"));
}

void BalanceSheetDialog::on_pos_mm_clicked()
{
    vertPos = Mid;
    horizPos = Mid;
    clearIcons();
    ui->pos_mm->setIcon(QIcon("://stop"));
}

void BalanceSheetDialog::on_pos_mr_clicked()
{
    vertPos = Mid;
    horizPos = Right;
    clearIcons();
    ui->pos_mr->setIcon(QIcon("://stop"));
}

void BalanceSheetDialog::on_pos_bl_clicked()
{
    vertPos = Down;
    horizPos = Left;
    clearIcons();
    ui->pos_bl->setIcon(QIcon("://stop"));
}

void BalanceSheetDialog::on_pos_bm_clicked()
{
    vertPos = Down;
    horizPos = Mid;
    clearIcons();
    ui->pos_bm->setIcon(QIcon("://stop"));
}

void BalanceSheetDialog::on_pos_br_clicked()
{
    vertPos = Down;
    horizPos = Right;
    clearIcons();
    ui->pos_br->setIcon(QIcon("://stop"));
}




























