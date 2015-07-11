#include "sheeteditorview.h"
#include <QMimeData>
#include <QDebug>

sheetEditorView::sheetEditorView(QWidget * parent) : QGraphicsView(parent)
{
    _pan = false;
    _panStartX = _panStartY = 0;
    setAcceptDrops(true);
}

sheetEditorView::~sheetEditorView()
{

}

void sheetEditorView::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void sheetEditorView::dropEvent(QDropEvent *e)
{
    QStringList sl;
    foreach(const QUrl &url, e->mimeData()->urls())
    {
        const QString &fileName = url.toLocalFile();
        sl.push_back(fileName);
    }
    droppedFiles(sl);
}

void sheetEditorView::dragMoveEvent(QDragMoveEvent *e)
{
    if(e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}
