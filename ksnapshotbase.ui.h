/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void KSnapshotBase::slotModeChanged( int i )
{
    switch ( i )
    {
    case 0:
	grpOptions->setEnabled(true);
	includeDecorations->setEnabled(false);
	break;
    case 1:
	grpOptions->setEnabled(true);
	includeDecorations->setEnabled(true);
	break;
    case 2:
	grpOptions->setEnabled(false);
    default:
	break;
    }
}
