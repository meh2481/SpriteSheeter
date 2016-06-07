#include "RecentDocuments.h"
#include <QDebug>
#include <QSettings>
#include <QStringList>

RecentDocuments::RecentDocuments(QObject *parent) : QObject(parent)
{
}

RecentDocuments::~RecentDocuments()
{
    QSettings settings("DaxarDev", "SpriteSheeterRecentDocuments");
    //Save list of recent documents
    char c = 'a';   //Use a char to sort for no real reason other than it works and it's fast
    foreach(QString file, mRecentFiles)
    {
        QString key;
        key.append(c++);
        settings.setValue(key, file);
    }
}

void RecentDocuments::addDocument(QString sFilename)
{
    int index = mRecentFiles.indexOf(sFilename);
    if(index >= 0)  //Currently exists in list; remove current location where it is
    {
        mRecentFiles.removeAt(index);
    }
    else    //Doesn't exist in list; remove from list if too many
    {
        if(mRecentFiles.size() >= NUM_RECENT_DOCUMENTS)
            mRecentFiles.removeLast();
    }

    //Add to start of list (so it'll be the first item in menu)
    mRecentFiles.prepend(sFilename);

    //Update menu in GUI
    updateMenu();
}

void RecentDocuments::init(QMenu* menu, QAction* before)
{
    QSettings settings("DaxarDev", "SpriteSheeterRecentDocuments");
    QStringList keys = settings.allKeys();

    //Create separator to insert everything before
    separator = menu->insertSeparator(before);

    keys.sort();    //Sort these so they're in most recent order

    foreach(QString key, keys)
    {
        QString value = settings.value(key).toString();
        if(value.size())
            mRecentFiles.append(value);
    }

    //Create menu
    for(int i = 0; i < NUM_RECENT_DOCUMENTS; i++)
    {
        QAction* act = new QAction(this);
        menuItems.append(act);
        QObject::connect(act, SIGNAL(triggered()), this, SLOT(openRecent()));
        menu->insertAction(separator, act);
    }

    //Update menu
    updateMenu();
}

void RecentDocuments::openRecent()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if(action)
        openFile(action->data().toString());
}

void RecentDocuments::updateMenu()
{
    if(!mRecentFiles.size())
        separator->setVisible(false);
    else
        separator->setVisible(true);

    int iCurItem = 0;
    foreach(QAction* act, menuItems)
    {
        if(iCurItem+1 <= mRecentFiles.size())
        {
            act->setVisible(true);
            act->setData(mRecentFiles.at(iCurItem));
            act->setText(QString::number(iCurItem+1) + ": " + mRecentFiles.at(iCurItem));
        }
        else
        {
            act->setVisible(false);
        }
        iCurItem++;
    }
}





























