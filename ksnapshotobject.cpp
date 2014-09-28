/*
 *  Copyright (C) 1997-2008 Richard J. Moore <rich@kde.org>
 *  Copyright (C) 2000 Matthias Ettrich <ettrich@troll.no>
 *  Copyright (C) 2002 Aaron J. Seigo <aseigo@kde.org>
 *  Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>
 *  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
 *  Copyright (C) 2006-2008 Urs Wolfer <uwolfer @ kde.org>
 *  Copyright (C) 2007 Montel Laurent <montel@kde.org>
 *  Copyright (C) 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
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

#include "ksnapshotobject.h"

#include "config-ksnapshot.h"

//Qt include
#include <QDebug>
#include <QRegExp>
#include <QApplication>
#include <QImageWriter>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTemporaryFile>

//kde include
#include <KIO/FileCopyJob>
#include <KIO/StatJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>

#if HAVE_X11
#include <fixx11h.h>
#endif

#include "regiongrabber.h"
#include "freeregiongrabber.h"

KSnapshotObject::KSnapshotObject()
    : m_regionGrab(0),
      m_freeRegionGrab(0),
      m_grabber(0)
{
}

KSnapshotObject::~KSnapshotObject()
{
    delete m_regionGrab;
    delete m_freeRegionGrab;
    delete m_grabber;
}

bool KSnapshotObject::urlExists(const QUrl &url, QWidget *window)
{
    if (!url.isValid()) {
        return false;
    }

    // we only need to test for existence; details about the file are uninteresting, so 0 for third param
    KIO::StatJob *job = KIO::stat(url, KIO::StatJob::DestinationSide, 0);
    KJobWidgets::setWindow(job, window);
    job->exec();

    return job->error() == KJob::NoError;
}

void KSnapshotObject::autoincFilenameUntilUnique(QWidget *window)
{
    forever {
        if (urlExists(m_filename, window)) {
            autoincFilename();
        } else {
            break;
        }
    }
}

void KSnapshotObject::autoincFilename()
{
    // Extract the m_filename from the path
    QString name = m_filename.fileName();

    // If the name contains a number then increment it
    QRegExp numSearch("(^|[^\\d])(\\d+)");    // we want to match as far left as possible, and when the number is at the start of the name

    // Does it have a number?
    int start = numSearch.lastIndexIn(name);
    if (start != -1) {
        // It has a number, increment it
        start = numSearch.pos(2);    // we are only interested in the second group
        QString numAsStr = numSearch.capturedTexts() [ 2 ];
        QString number = QString::number(numAsStr.toInt() + 1);
        number = number.rightJustified(numAsStr.length(), '0');
        name.replace(start, numAsStr.length(), number);
    } else {
        // no number
        start = name.lastIndexOf('.');
        if (start != -1) {
            // has a . somewhere, e.g. it has an extension
            name.insert(start, '1');
        } else {
            // no extension, just tack it on to the end
            name += '1';
        }
    }

    //Rebuild the path
    QUrl newUrl = m_filename;
    newUrl = newUrl.adjusted(QUrl::RemoveFilename);
    newUrl.setPath(newUrl.path() + name);
    changeUrl(newUrl.url());
}


void KSnapshotObject::changeUrl(const QUrl &url)
{
    if (url == m_filename) {
        return;
    }

    m_filename = url;
    refreshCaption();
}


// NOTE: window == NULL if called from dbus interface
bool KSnapshotObject::save(const QString &filename, QWidget *window)
{
    return save(QUrl::fromUserInput(filename), window);
}

bool KSnapshotObject::save(const QUrl &url, QWidget *window)
{
    // NOTE: window == NULL if called from dbus interface
    //TODO: non-blocking

    // we only need to test for existence; details about the file are uninteresting, so 0 for third param
    if (urlExists(url, window)) {
        const QString title = i18n("File Exists");
        const QString text = i18n("<qt>Do you really want to overwrite <b>%1</b>?</qt>" , url.url(QUrl::PreferLocalFile));
        if (KMessageBox::Continue != KMessageBox::warningContinueCancel(window, text, title, KGuiItem(i18n("Overwrite")))) {
            return false;
        }
    }

    const bool success = saveTo(url, window);
    if (success) {
        m_successfulSaveUrl = url;
        m_filename = url;
        autoincFilename();
        refreshCaption();
    } else {
        m_successfulSaveUrl = QUrl();
    }

    return success;
}

bool KSnapshotObject::saveTo(const QUrl &url, QWidget *window)
{
    QMimeDatabase db;
    QString type = db.mimeTypeForUrl(url).preferredSuffix();
    if (type.isEmpty()) {
        type = "PNG";
    }

    bool ok = false;

    if (url.isLocalFile()) {
        QFile output(url.toLocalFile());
        if (output.open(QFile::WriteOnly)) {
            ok = saveImage(&output, type.toLatin1());
        }
    } else {
        QTemporaryFile tmpFile;
        if (tmpFile.open() && saveImage(&tmpFile, type.toLatin1())) {
            // TODO: non-blocking
            KIO::FileCopyJob *job = KIO::file_copy(tmpFile.fileName(), url);
            KJobWidgets::setWindow(job, window);
            job->exec();
            ok = job->error() == KJob::NoError;
        }
    }

    QApplication::restoreOverrideCursor();
    if (!ok) {
        qWarning() << "KSnapshot was unable to save the m_snapshot to" << url << "type: " << type;

        const QString caption = i18n("Unable to Save Image");
        const QString text = i18n("KSnapshot was unable to save the image to\n%1.", url.toDisplayString());
        KMessageBox::error(window, text, caption);
    }

    return ok;
}

bool KSnapshotObject::saveImage(QIODevice *device, const QByteArray &format)
{
    QImageWriter imgWriter(device, format);

    if (!imgWriter.canWrite()) {
        qDebug() << "Cannot write format " << format;
        return false;
    }

    qDebug() << "saving file format" << format << " in quality " << imgWriter.quality();

    // For jpeg and webp use 85% quality not the default
    if (0 == qstricmp(format.constData(), "jpeg")
            || 0 == qstricmp(format.constData(), "jpg")
            || 0 == qstricmp(format.constData(), "webp")) {
        imgWriter.setQuality(85);
    }

    if (!m_title.isEmpty()) {
        imgWriter.setText(i18n("Title"), m_title);
    }

    if (!m_windowClass.isEmpty()) {
        imgWriter.setText(i18n("Window Class"), m_windowClass);
    }

    QImage snap = m_snapshot.toImage();
    return imgWriter.write(snap);
}
