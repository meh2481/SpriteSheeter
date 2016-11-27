#ifndef BALANCESHEET_H
#define BALANCESHEET_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include "BalancePos.h"

namespace Ui {
    class balanceSheet;
}

class BalanceSheetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BalanceSheetDialog(QWidget *parent = 0);
    ~BalanceSheetDialog();

signals:

    void balance(int w, int h, BalancePos::Pos vert, BalancePos::Pos horiz);

public slots:
    void defaultWH(int w, int h);

private slots:

    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void on_pos_ul_clicked();
    void on_pos_um_clicked();
    void on_pos_ur_clicked();
    void on_pos_ml_clicked();
    void on_pos_mm_clicked();
    void on_pos_mr_clicked();
    void on_pos_bl_clicked();
    void on_pos_bm_clicked();
    void on_pos_br_clicked();

private:
    Ui::balanceSheet *ui;

    BalancePos::Pos vertPos;
    BalancePos::Pos horizPos;

    void clearIcons();
};

#endif // BALANCESHEET_H



















