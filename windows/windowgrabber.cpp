/*
  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
  Copyright (C) 2010, 2011 Pau Garcia i Quiles <pgquiles@elpauer.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or ( at your option ) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "windowgrabber.h"

#include <windows.h>

static UINT cxWindowBorder, cyWindowBorder;

static
const int minSize = 8;

static
bool operator< (const QRect &r1, const QRect &r2)
{
    return r1.width() * r1.height() < r2.width() * r2.height();
}

static
bool maybeAddWindow(HWND hwnd, std::vector<QRect> *windows)
{
    WINDOWINFO wi;
    GetWindowInfo(hwnd, &wi);
    RECT rect = wi.rcClient;

#if 0
    RECT rect;
    GetWindowRect(hwnd, &rect);
#endif

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // For some reason, rect.left and rect.top are shifted by cxWindowBorders and cyWindowBorders pixels respectively
    // in *every* case (for every window), but cxWindowBorders and cyWindowBorders are non-zero only in the
    // biggest-window case, therefore we need to save the biggest cxWindowBorders and cyWindowBorders to adjust the rect later
    cxWindowBorder = qMax(cxWindowBorder, wi.cxWindowBorders);
    cyWindowBorder = qMax(cyWindowBorder, wi.cyWindowBorders);

    if (((wi.dwStyle & WS_VISIBLE) != 0) && (width >= minSize) && (height >= minSize)) {
        //QRect r( rect.left + 4, rect.top + 4, width, height); // 4 = max(wi.cxWindowBorders) = max(wi.cyWindowBorders)
        QRect r(rect.left + cxWindowBorder, rect.top + cyWindowBorder, width, height);
        if (std::find(windows->begin(), windows->end(), r) == windows->end()) {
            windows->push_back(r);
            return true;
        }
    }
    return false;
}

static
BOOL CALLBACK getWindowsRecursiveHelper(HWND hwnd, LPARAM lParam)
{
    maybeAddWindow(hwnd, reinterpret_cast< std::vector<QRect>* >(lParam));
    return TRUE;
}

static
void getWindowsRecursive(std::vector<QRect> *windows, HWND hwnd,
                         int rx = 0, int ry = 0, int depth = 0)
{

    maybeAddWindow(hwnd, windows);

    EnumChildWindows(hwnd, getWindowsRecursiveHelper, (LPARAM) windows);

    std::sort(windows->begin(), windows->end());
}

static
HWND findRealWindow(HWND w, int depth = 0)
{
    // TODO Implement
    return w; // This is WRONG but makes code compile for now
}

static
HWND windowUnderCursor(bool includeDecorations = true)
{
    POINT pointCursor;
    QPoint qpointCursor = QCursor::pos();
    pointCursor.x = qpointCursor.x();
    pointCursor.y = qpointCursor.y();
    HWND windowUnderCursor = WindowFromPoint(pointCursor);

    if (includeDecorations) {
        LONG_PTR style = GetWindowLongPtr(windowUnderCursor, GWL_STYLE);
        if ((style & WS_CHILD) != 0) {
            windowUnderCursor = GetAncestor(windowUnderCursor, GA_ROOT);
        }
    }
    return windowUnderCursor;
}

static
QPixmap grabWindow(HWND hWnd, QString *title = 0, QString *windowClass = 0)
{
    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);
    int w = windowRect.right - windowRect.left;
    int h = windowRect.bottom - windowRect.top;
    HDC targetDC = GetWindowDC(hWnd);
    HDC hDC = CreateCompatibleDC(targetDC);
    HBITMAP tempPict = CreateCompatibleBitmap(targetDC, w, h);
    HGDIOBJ oldPict = SelectObject(hDC, tempPict);
    BitBlt(hDC, 0, 0, w, h, targetDC, 0, 0, SRCCOPY);
    tempPict = (HBITMAP) SelectObject(hDC, oldPict);
    QPixmap pm = QPixmap::fromWinHBITMAP(tempPict);

    DeleteDC(hDC);
    ReleaseDC(hWnd, targetDC);

    KWindowInfo winInfo(findRealWindow(hWnd), NET::WMVisibleName, NET::WM2WindowClass);
    if (title) {
        (*title) = winInfo.visibleName();
    }

    if (windowClass) {
        (*windowClass) = winInfo.windowClassName();
    }
    return pm;
}

QPixmap WindowGrabber::platformSetup()
{
    HWND child = windowUnderCursor();

    WINDOWINFO wi;
    GetWindowInfo(child, &wi);

    RECT r;
    GetWindowRect(child, &r);
    x = r.left;
    y = r.top;
    w = r.right - r.left;
    h = r.bottom - r.top;
    cxWindowBorder = wi.cxWindowBorders;
    cyWindowBorder = wi.cyWindowBorders;

    HDC childDC = GetDC(child);

    QPixmap pm(grabWindow(child, &title, &windowClass));
    getWindowsRecursive(&windows, child);
    return pm;
}

QPixmap WindowGrabber::grabCurrent(bool includeDecorations)
{
    int x, y;
    HWND hWindow;
    hWindow = windowUnderCursor(includeDecorations);
    Q_ASSERT(hWindow);

    HWND hParent;

    // Now find the top-most window
    do {
        hParent = hWindow;
    } while ((hWindow = GetParent(hWindow)) != NULL);
    Q_ASSERT(hParent);

    RECT r;
    GetWindowRect(hParent, &r);

    x = r.left;
    y = r.top;

    windowPosition = QPoint(x, y);
    QPixmap pm(grabWindow(hParent, &title, &windowClass));
    return pm;
}

