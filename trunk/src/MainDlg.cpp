#include "StdAfx.h"
#include "Resource.h"
#include "MainDlg.h"

#include "URLDlg.h"

CMainDlg::CMainDlg(void) : m_bThreadRunning(false)
	, m_bDragMode(false)
	, m_oldx(-1)
	, m_oldy(-1)
{
}

CMainDlg::~CMainDlg(void)
{
}

LRESULT CMainDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hwndDlg, IDI_COMMITMONITOR);
		}
		return TRUE;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam));
	case WM_NOTIFY:
		{
		}
		break;
	case WM_SETCURSOR:
		{
			return OnSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
		}
		break;
	case WM_MOUSEMOVE:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			return OnMouseMove(wParam, pt);
		}
		break;
	case WM_LBUTTONDOWN:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			return OnLButtonDown(wParam, pt);
		}
		break;
	case WM_LBUTTONUP:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			return OnLButtonUp(wParam, pt);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CMainDlg::DoCommand(int id)
{
	switch (id)
	{
	case IDOK:
		break;
	case IDCANCEL:
		PostQuitMessage(IDCANCEL);
		return 0;
		break;
	case IDC_ADDURL:
	case IDC_URLEDIT:
		CURLDlg dlg;
		dlg.DoModal(hResource, IDD_URLCONFIG, *this);
		return 0;
		break;
	}
	return 1;
}

/******************************************************************************/
/* tree and list view resizing                                                */
/******************************************************************************/

bool CMainDlg::OnSetCursor(HWND hWnd, UINT nHitTest, UINT message) 
{
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(nHitTest);
	if (m_bThreadRunning)
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
		SetCursor(hCur);
		return TRUE;
	}
	if (hWnd == *this)
	{
		RECT rect;
		POINT pt;
		GetClientRect(*this, &rect);
		GetCursorPos(&pt);
		ScreenToClient(*this, &pt);
		if (PtInRect(&rect, pt))
		{
			ClientToScreen(*this, &pt);
			// are we right of the tree control?

			::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &rect);
			if ((pt.x > rect.right)&&
				(pt.y >= rect.top)&&
				(pt.y <= rect.bottom))
			{
				// but left of the list control?
				::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &rect);
				if (pt.x < rect.left)
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
					SetCursor(hCur);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

bool CMainDlg::OnMouseMove(UINT nFlags, POINT point)
{
	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient;

	if (m_bDragMode == false)
		return false;

	// create an union of the tree and list control rectangle
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);
	point.x -= rect.left;
	point.y -= treelist.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if ((nFlags & MK_LBUTTON) && (point.x != m_oldx))
	{
		hDC = GetDC(*this);

		if (hDC)
		{
			DrawXorBar(hDC, m_oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
			DrawXorBar(hDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);

			ReleaseDC(*this, hDC);
		}

		m_oldx = point.x;
		m_oldy = point.y;
	}

	return true;
}

bool CMainDlg::OnLButtonDown(UINT nFlags, POINT point)
{
	UNREFERENCED_PARAMETER(nFlags);

	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient;

	// create an union of the tree and list control rectangle
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);
	point.x -= rect.left;
	point.y -= treelist.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if ((point.y < treelist.top) || 
		(point.y > treelist.bottom))
		return false;

	m_bDragMode = true;

	SetCapture(*this);

	hDC = GetDC(*this);
	DrawXorBar(hDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
	ReleaseDC(*this, hDC);

	m_oldx = point.x;
	m_oldy = point.y;

	return true;
}

bool CMainDlg::OnLButtonUp(UINT nFlags, POINT point)
{
	UNREFERENCED_PARAMETER(nFlags);

	HDC hDC;
	RECT rect, tree, list, treelist, treelistclient;

	if (m_bDragMode == FALSE)
		return false;

	// create an union of the tree and list control rectangle
	::GetWindowRect(::GetDlgItem(*this, IDC_MONITOREDURLS), &list);
	::GetWindowRect(::GetDlgItem(*this, IDC_URLTREE), &tree);
	UnionRect(&treelist, &tree, &list);
	treelistclient = treelist;
	MapWindowPoints(NULL, *this, (LPPOINT)&treelistclient, 2);

	ClientToScreen(*this, &point);
	GetClientRect(*this, &rect);
	MapWindowPoints(*this, NULL, (LPPOINT)&rect, 2);

	POINT point2 = point;
	if (point2.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point2.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point2.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point2.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	point.x -= rect.left;
	point.y -= treelist.top;

	OffsetRect(&treelist, -treelist.left, -treelist.top);

	if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	hDC = GetDC(*this);
	DrawXorBar(hDC, m_oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);			
	ReleaseDC(*this, hDC);

	m_oldx = point.x;
	m_oldy = point.y;

	m_bDragMode = false;
	ReleaseCapture();

	//position the child controls
	HDWP hdwp = BeginDeferWindowPos(2);
	if (hdwp)
	{
		GetWindowRect(GetDlgItem(*this, IDC_URLTREE), &treelist);
		treelist.right = point2.x - 2;
		MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
		DeferWindowPos(hdwp, GetDlgItem(*this, IDC_URLTREE), NULL, 
			treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);

		GetWindowRect(GetDlgItem(*this, IDC_MONITOREDURLS), &treelist);
		treelist.left = point2.x + 2;
		MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
		DeferWindowPos(hdwp, GetDlgItem(*this, IDC_MONITOREDURLS), NULL, 
			treelist.left, treelist.top, treelist.right-treelist.left, treelist.bottom-treelist.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
		EndDeferWindowPos(hdwp);
	}

	return true;
}

void CMainDlg::DrawXorBar(HDC hDC, LONG x1, LONG y1, LONG width, LONG height)
{
	static WORD _dotPatternBmp[8] = 
	{ 
		0x0055, 0x00aa, 0x0055, 0x00aa, 
		0x0055, 0x00aa, 0x0055, 0x00aa
	};

	HBITMAP hbm;
	HBRUSH  hbr, hbrushOld;

	hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
	hbr = CreatePatternBrush(hbm);

	SetBrushOrgEx(hDC, x1, y1, NULL);
	hbrushOld = (HBRUSH)SelectObject(hDC, hbr);

	PatBlt(hDC, x1, y1, width, height, PATINVERT);

	SelectObject(hDC, hbrushOld);

	DeleteObject(hbr);
	DeleteObject(hbm);
}
