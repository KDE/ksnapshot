/*
 *  Copyright (C) 2007 Montel Laurent <montel@kde.org>
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

#include <kcmdlineargs.h>
#include <klocale.h>


static KCmdLineOptions ksnapshot_options[] =
{
    { "c", 0, 0 },
    { "current", I18N_NOOP("Captures the window under the mouse on startup (instead of the desktop)"), 0 },
    { "fullscreen", I18N_NOOP("Captures the desktop"), 0 },
    { "region", I18N_NOOP("Captures a region"), 0 },
    { "child", I18N_NOOP("Captures a part of windows"), 0 },
    { 0, 0, 0 }
};

#endif
