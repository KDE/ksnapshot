/*
 *  Copyright (C) 2007 Montel Laurent <montel@kde.org>
 *  Copyright (C) 2014 Aaron Seigo <aseigo@kde.org>
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

#ifndef KSNAPSHOTOPTIONS_H
#define KSNAPSHOTOPTIONS_H

#include <QCommandLineParser>

#include <KLocalizedString>

static void addCommandLineOptions(QCommandLineParser &parser)
{
    QCommandLineOption currentOption(QStringList() << "c" << "current",
                                     i18n("Captures the window under the mouse on startup (instead of the desktop)"));
    parser.addOption(currentOption);

    QCommandLineOption fullscreenOption("fullscreen", i18n("Captures the desktop"));
    parser.addOption(fullscreenOption);

    QCommandLineOption regionOption("region", i18n("Captures a region"));
    parser.addOption(regionOption);

    QCommandLineOption freeRegionOption("freeregion", i18n("Captures a free region (not rectangular)"));
    parser.addOption(freeRegionOption);

    QCommandLineOption childOption("child", i18n("Captures a part of windows"));
    parser.addOption(childOption);
}

#endif
