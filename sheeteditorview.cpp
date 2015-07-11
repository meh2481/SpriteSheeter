#include "sheeteditorview.h"
#include <QMimeData>
#include <QDebug>
#include <QDir>

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
    QStringList sFiles;
    QStringList sFolders;
    foreach(const QUrl &url, e->mimeData()->urls())
    {
        const QString &fileName = url.toLocalFile();
        if(QDir(fileName).exists())
            sFolders.push_back(fileName);
        else
            sFiles.push_back(fileName);
    }
    droppedFiles(sFiles);
    droppedFolders(sFolders);
}

void sheetEditorView::dragMoveEvent(QDragMoveEvent *e)
{
    if(e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}
