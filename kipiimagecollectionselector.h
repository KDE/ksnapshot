// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Copyright 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
based on code for Gwenview by
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef KIPIIMAGECOLLECTIONSELECTOR_H
#define KIPIIMAGECOLLECTIONSELECTOR_H

// KIPI
#include <libkipi/imagecollection.h>
#include <libkipi/imagecollectionselector.h>

// Local
#include "kipiinterface.h"

struct KIPIImageCollectionSelectorPrivate;
class KIPIImageCollectionSelector : public KIPI::ImageCollectionSelector {
	Q_OBJECT
public:
	KIPIImageCollectionSelector(KIPIInterface*, QWidget* parent);
	~KIPIImageCollectionSelector();

	virtual QList<KIPI::ImageCollection> selectedImageCollections() const;

private:
	KIPIImageCollectionSelectorPrivate* const d;
};

#endif /* KIPIIMAGECOLLECTIONSELECTOR_H */
