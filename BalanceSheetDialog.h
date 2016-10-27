#ifndef BALANCESHEET_H
#define BALANCESHEET_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

namespace Ui {
class balanceSheet;
}

class BalanceSheetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BalanceSheetDialog(QWidget *parent = 0);
    ~BalanceSheetDialog();

    typedef int Pos;
    static const Pos Up = 0;
    static const Pos Mid = 1;
    static const Pos Down = 2;
    static const Pos Left = 0;
    static const Pos Right = 2;

signals:

    void balance(int w, int h, BalanceSheetDialog::Pos vert, BalanceSheetDialog::Pos horiz);

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

    Pos vertPos;
    Pos horizPos;

    void clearIcons();
};

#endif // BALANCESHEET_H



















