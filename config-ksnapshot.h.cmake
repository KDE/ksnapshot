#ifndef CONFIG_KSNAPSHOT_H
#define CONFIG_KSNAPSHOT_H

/* Define to 1 if we are building with X11 */
#cmakedefine HAVE_X11 1

/* Define to 1 if you have xcb */
#cmakedefine XCB_XCB_FOUND 1

/* Define to 1 if you have x11-xcb */
#cmakedefine X11_XCB_FOUND 1

/* Define to 1 if you have xfixes */
#cmakedefine XCB_XFIXES_FOUND 1

/* Define to 1 if you have shape */
#cmakedefine XCB_SHAPE_FOUND 1

/* Define to 1 if you have libkipi */
#cmakedefine KIPI_FOUND 1

/* Set the KSnapshot version from CMake */
#cmakedefine KSNAPVERSION "@KSNAPVERSION@"

#endif