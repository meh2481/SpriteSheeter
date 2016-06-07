#ifndef RECENTDOCUMENTS_H
#define RECENTDOCUMENTS_H

#include <QObject>
#include <QStringList>
#include <QMenu>
#include <QAction>
#include <QList>

#define NUM_RECENT_DOCUMENTS 10

class RecentDocuments : public QObject
{
    Q_OBJECT

    QStringList mRecentFiles;   //First item in list is most recent document
    QList<QAction*> menuItems;
    QAction* separator;

    void updateMenu();
public:
    explicit RecentDocuments(QObject *parent = 0);
    ~RecentDocuments();

    void addDocument(QString sFilename);
    void init(QMenu* menu, QAction* before);

signals:
    void openFile(QString sFilename);

public slots:
    void openRecent();
};

#endif // RECENTDOCUMENTS_H
