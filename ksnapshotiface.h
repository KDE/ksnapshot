/**     KSnapshot DCOP interface
        File: ksnapshotiface.h
        Date: January 12, 2001
        Author: Ian Geiser <geiseri@linuxppc.com>
        Comments:
                This is an addition to the existing KSnapshot code
                that will allow other applications to access internal
                public member functions via dcop.
**/

#ifndef __KS_IFACE_H
#define __KS_IFACE_H

#include <dcopobject.h>

class KSnapshotIface : virtual public DCOPObject
{
        K_DCOP
        k_dcop:
	/** the current filename (as a URL) that will
	    be used to save to */
	virtual QString url() const = 0;

        /** Grab an image **/
        virtual void slotGrab() = 0;

	/** Prints the image. */
	virtual void slotPrint() = 0;

        /** Saves the image **/
        virtual void slotSave() = 0;

	/** Save the image to the specified filename */
        virtual bool save(const QString &filename) = 0;

        /** Saves image as **/
        virtual void slotSaveAs() = 0;

        /** Copy the snapshot to the clipboard. **/
	virtual void slotCopy() = 0;

        /** Set the timeout value */
        virtual void setTime(int newTime) = 0;

	/** Get the current timeout value */
	virtual int timeout() = 0;

        /** Set the URL to the file to save **/
        virtual void setURL(const QString &newURL) = 0;

        /** Set the ability to grab the entire screen, just the window
                containing the mouse, or a region */
        virtual void setGrabMode(int grab) = 0;

	/** Return the current grab mode */
	virtual int grabMode() = 0;

	/** Move the mouse pointer. */
	virtual void slotMovePointer( int x, int y ) = 0;

        /** Exit KSnapshot **/
	virtual void exit() = 0;
};

#endif
