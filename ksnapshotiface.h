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
        /** Grab an image **/
        virtual void slotGrab() = 0;
        /** Save an image **/
        virtual void slotSave() = 0;
        /** Set the timout value **/
        virtual void setTime(int newTime) = 0;
        /** Set the URL to the file to save **/
        virtual void setURL(QString newURL) = 0;
        /** Set the ability to grab the entire screen or just the window
                containing the mouse **/
        virtual void setGrabPointer(bool grab) = 0;
        /** Exit KSnapshot **/
	virtual void exit() = 0;
};

#endif
