#include "NameVisibleStep.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

NameVisibleStep::NameVisibleStep(MainWindow* w, bool visible) : UndoStep(w)
{
    newVisible = visible;
}

void NameVisibleStep::undo()
{
    Ui::MainWindow* ui = mainWindow->getUI();
    Sheet* sheet = mainWindow->getSheet();
    ui->animationNameEditor->setEnabled(!newVisible);
    ui->fontButton->setEnabled(!newVisible);
    ui->fontColSelect->setEnabled(!newVisible);
    sheet->setNamesVisible(!newVisible);
    sheet->refresh();
    mainWindow->updateSelectedAnim();
    ui->animNameEnabled->setChecked(!newVisible);
}

void NameVisibleStep::redo()
{
    Ui::MainWindow* ui = mainWindow->getUI();
    Sheet* sheet = mainWindow->getSheet();
    ui->animationNameEditor->setEnabled(newVisible);
    ui->fontButton->setEnabled(newVisible);
    ui->fontColSelect->setEnabled(newVisible);
    sheet->setNamesVisible(newVisible);
    sheet->refresh();
    mainWindow->updateSelectedAnim();
    ui->animNameEnabled->setChecked(newVisible);
}
