#include "sheeteditorview.h"

sheetEditorView::sheetEditorView(QWidget * parent) : QGraphicsView(parent)
{
    _pan = false;
    _panStartX = _panStartY = 0;
}

sheetEditorView::~sheetEditorView()
{

}
