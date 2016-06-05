#include "SheetEditorView.h"
#include <QMimeData>
#include <QDebug>
#include <QDir>

SheetEditorView::SheetEditorView(QWidget * parent) : QGraphicsView(parent)
{
    _pan = false;
    _panStartX = _panStartY = 0;
    setAcceptDrops(true);
}

SheetEditorView::~SheetEditorView()
{

}

void SheetEditorView::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void SheetEditorView::dropEvent(QDropEvent *e)
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

void SheetEditorView::dragMoveEvent(QDragMoveEvent *e)
{
    if(e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}
