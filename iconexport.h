#ifndef ICONEXPORT_H
#define ICONEXPORT_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

#define ICON_WIDTH 148
#define ICON_HEIGHT 125

#define MIN_ICON_SZ 5.0f

namespace Ui {
class iconExport;
}

class iconExport : public QDialog
{
    Q_OBJECT

public:
    explicit iconExport(QWidget *parent = 0);
    ~iconExport();

signals:

public slots:
    void setImage(QImage img);

private slots:

    void on_fitXButton_clicked();

    void on_fitYButton_clicked();

    void on_resetButton_clicked();

    void on_horizontalSlider_sliderMoved(int position);

    void on_offsetXBox_valueChanged(int arg1);

    void on_offsetYBox_valueChanged(int arg1);

    void on_centerButton_clicked();

    void on_saveIconBtn_clicked();

    void on_cancelBtn_clicked();

    void on_horizontalSlider_valueChanged(int value);

private:
    Ui::iconExport *ui;
    QImage iconImg;
    QImage *transparentBg;

    QString lastIconStr;

    float iconScale;

    bool bMouseDown;
    int lastx;
    int lasty;

    QGraphicsScene* scene;
    QGraphicsPixmapItem* item;

    void drawPreview();
    void updateScaleText();
    void saveSettings();
    void loadSettings();

    bool eventFilter(QObject *obj, QEvent *event);

};

#endif // ICONEXPORT_H
