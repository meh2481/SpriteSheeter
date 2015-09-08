#ifndef BALANCESHEET_H
#define BALANCESHEET_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

namespace Ui {
class balanceSheet;
}

class balanceSheet : public QDialog
{
    Q_OBJECT

public:
    explicit balanceSheet(QWidget *parent = 0);
    ~balanceSheet();

    typedef int Pos;
    const Pos Up = 0;
    const Pos Mid = 1;
    const Pos Down = 2;
    const Pos Left = 0;
    const Pos Right = 2;

signals:

public slots:

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


};

#endif // BALANCESHEET_H
