#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>

namespace Ui {
class importDialog;
}

class importDialog : public QDialog
{
    Q_OBJECT

public:
    explicit importDialog(QWidget *parent = 0);
    ~importDialog();

private slots:
    void on_okButton_clicked();

    void on_pushButton_2_clicked();

    void on_allButton_clicked();

private:
    Ui::importDialog *ui;
};

#endif // IMPORTDIALOG_H
