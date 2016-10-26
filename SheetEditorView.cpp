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

void SheetEditorView::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::MidButton || event->button() == Qt::RightButton)
    {
        _pan = true;
        _panStartX = event->x();
        _panStartY = event->y();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if(event->button() == Qt::LeftButton)
    {
        QPointF pt = mapToScene(event->pos());
        mousePressed(pt.x(), pt.y());
    }
    event->ignore();
}

void SheetEditorView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::MidButton || event->button() == Qt::RightButton)
    {
        _pan = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    if(event->button() == Qt::LeftButton)
    {
        QPointF pt = mapToScene(event->pos());
        mouseReleased(pt.x(), pt.y());
    }
    event->ignore();
}

void SheetEditorView::mouseMoveEvent(QMouseEvent *event)
{
    if(_pan)
    {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - _panStartX));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - _panStartY));
        _panStartX = event->x();
        _panStartY = event->y();
        event->accept();
        return;
    }
    QPointF pt = mapToScene(event->pos());
    mouseMoved(pt.x(), pt.y());
    event->ignore();
}


















