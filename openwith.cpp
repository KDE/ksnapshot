/*
 *  Copyright (C) 2006 Marcus Hufgard <marcus.Hufgard@hufgard.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */


#include <QStandardItemModel>

#include <kopenwith.h>
#include <kservice.h>
#include <klocalizedstring.h>
#include <kvbox.h>
#include <kpushbutton.h>
#include <kmimetypetrader.h>
#include <kicon.h>

#include "openwith.h"

OpenWith::OpenWith(const QString& mimeType, const QString& value, QWidget *parent)
 : KDialog(parent), mime(mimeType), value1(value)
{
     setCaption(i18n("OpenWith"));
     setButtons(Ok|Cancel);
     setDefaultButton(Ok);
     enableButton(Ok, false);

     KVBox* vbox1 = new KVBox(this);
     setMainWidget(vbox1);
     vbox1->setSpacing(spacingHint());

     // Show the diferent applications 

     applistview= new AppListView(this, vbox1);

     KService::List  apps;
     
     apps = KMimeTypeTrader::self()->query( "image/png");
     if (!apps.isEmpty())
     { 
         QStandardItemModel *data = new QStandardItemModel(apps.count(), 3);
         KService::List::ConstIterator it = apps.begin();
         int count=0;
         for(; it != apps.end(); it++, count++)
         {
	     KService::Ptr service = (*it);
             QStandardItem* item1 = new QStandardItem(KIcon(service->icon()), service->icon());
             data->setItem(count, 0, item1);
             QStandardItem* item2 = new QStandardItem(service->name().replace( "&", "&&" ));
             data->setItem(count, 1, item2);
             QStandardItem* item3 = new QStandardItem(service);
             data->setItem(count, 2, item3);
	 }    
         applistview->setModel(data);
     }    

     KPushButton* otherButton = new KPushButton(i18n( "Other application" ), vbox1);
     connect(otherButton, SIGNAL(clicked()), SLOT(otherApplication()));
     QFrame* frame1 = new QFrame(vbox1);
     frame1->setFrameShape(QFrame::HLine);
   
}

OpenWith::~OpenWith()
{
}

void OpenWith::otherApplication()
{
    KUrl::List list;

    list.append(value1);

    KOpenWithDlg* dlg = new KOpenWithDlg(mime, QString(), this);
    bool runok;
    runok = dlg->exec();
    if (runok && dlg->service())
    {
         KRun::run(*dlg->service(), list, this);
    } else {
         if (!dlg->service() && runok && dlg->text() != "")
             KRun::run(dlg->text(), list);
    }
    if (runok)
        close();
}

void OpenWith::runSelected()
{
    KUrl::List list;
    list.append(value1);

    QModelIndexList indexes = applistview->selectionModel()->selectedIndexes();
    QModelIndex index;
    QString exec;

    foreach(index, indexes) 
        exec = QString("%1").arg(index.model()->data(index, 2).toString());
    
    KRun::run(exec, list); 
}


void OpenWith::accept()
{
    runSelected();
    close();
}

// ************** class AppListView *******************************************/

AppListView::AppListView(OpenWith *dlg, QWidget* parent)
    :QListView(parent), openwdlg(dlg)
{
    setEditTriggers(QListView::NoEditTriggers);
    setViewMode(QListView::ListMode);
    setSelectionMode(QListView::SingleSelection);
}

AppListView::~AppListView()
{
}

void AppListView::selectionChanged(const QItemSelection&, const QItemSelection&)
{
     openwdlg->enableButton(OpenWith::Ok, true);
}

void AppListView::mouseDoubleClickEvent(QMouseEvent*)
{
    openwdlg->runSelected();
    openwdlg->close();
}


#include "openwith.moc"
