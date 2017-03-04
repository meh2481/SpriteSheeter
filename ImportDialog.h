#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QImage>

namespace Ui {
class importDialog;
}

class ImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportDialog(QWidget *parent = 0);
    ~ImportDialog();

    void showEvent(QShowEvent *);
    void resizeEvent(QResizeEvent* event);

signals:
    void importOK(QImage curImage, int numx, int numy, bool bVert, bool bSplit);
    void importAll(QImage curImage, int numx, int numy, bool bVert, bool bSplit);

public slots:
    bool setPreviewImage(QImage image);

private slots:
    void on_okButton_clicked();
    void on_allButton_clicked();

    void on_cancelButton_clicked();

private:
    Ui::importDialog *ui;

    QGraphicsScene* scene;
    QGraphicsPixmapItem* item;

    QImage importImage;

    void saveSettings();
    void restoreSettings();
};

#endif // IMPORTDIALOG_H
