#include "StdAfx.h"
#include "Resource.h"
#include "MainDlg.h"

#include "URLDlg.h"
#include "AppUtils.h"


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
			wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
			if (PathFileExists(urlfile.c_str()))
				m_URLInfos.Load(urlfile.c_str());
			RefreshURLTree();
		}
		break;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam));
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
		EndDialog(*this, IDCANCEL);
		break;
	case IDC_URLEDIT:
		{
			CURLDlg dlg;
			HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
			HTREEITEM hItem = TreeView_GetSelection(hTreeControl);
			if (hItem)
			{
				TVITEMEX itemex = {0};
				itemex.hItem = hItem;
				itemex.mask = TVIF_PARAM;
				TreeView_GetItem(hTreeControl, &itemex);
				if (m_URLInfos.infos.find(*(wstring*)itemex.lParam) != m_URLInfos.infos.end())
				{
					dlg.SetInfo(&m_URLInfos.infos[*(wstring*)itemex.lParam]);
					dlg.DoModal(hResource, IDD_URLCONFIG, *this);
					CUrlInfo * inf = dlg.GetInfo();
					if ((inf)&&inf->url.size())
					{
						m_URLInfos.infos[inf->url] = *inf;
					}
					SaveURLInfo();
					RefreshURLTree();
				}
			}
		}
		break;
	case IDC_ADDURL:
		{
			CURLDlg dlg;
			dlg.DoModal(hResource, IDD_URLCONFIG, *this);
			CUrlInfo * inf = dlg.GetInfo();
			if ((inf)&&inf->url.size())
			{
				m_URLInfos.infos[inf->url] = *inf;
			}
			SaveURLInfo();
			RefreshURLTree();
		}
		break;
	default:
		return 0;
	}
	return 1;
}

/******************************************************************************/
/* data persistence                                                           */
/******************************************************************************/
void CMainDlg::SaveURLInfo()
{
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	m_URLInfos.Save(urlfile.c_str());
}

void CMainDlg::LoadURLInfo()
{
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	if (PathFileExists(urlfile.c_str()))
		m_URLInfos.Load(urlfile.c_str());
	RefreshURLTree();
}

/******************************************************************************/
/* tree handling                                                */
/******************************************************************************/
void CMainDlg::RefreshURLTree()
{
	// the m_URLInfos member must be up-to-date here

	HWND hTreeControl = GetDlgItem(*this, IDC_URLTREE);
	// first clear the tree control
	TreeView_DeleteAllItems(hTreeControl);
	// now add a tree item for every entry in m_URLInfos
	for (map<wstring, CUrlInfo>::const_iterator it = m_URLInfos.infos.begin(); it != m_URLInfos.infos.end(); ++it)
	{
		TVINSERTSTRUCT tv = {0};
		tv.hParent = TVI_ROOT;
		tv.hInsertAfter = TVI_SORT;
		tv.itemex.mask = TVIF_TEXT|TVIF_PARAM;
		WCHAR * str = new WCHAR[it->second.name.size()+10];
		// find out if there are some unread entries
		int unread = 0;
		for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = it->second.logentries.begin(); logit != it->second.logentries.end(); ++logit)
		{
			if (!logit->second.read)
				unread++;
		}
		if (unread)
		{
			_stprintf_s(str, it->second.name.size()+10, _T("%s (%d)"), it->second.name.c_str(), unread);
		}
		else
			_tcscpy_s(str, it->second.name.size()+1, it->second.name.c_str());

		tv.itemex.pszText = str;
		tv.itemex.lParam = (LPARAM)&it->first;
		HTREEITEM hItem = TreeView_InsertItem(hTreeControl, &tv);
		delete [] str;
		//if ((hItem)&&(it->second.subentries.size()))
		//{
		//	for (map<wstring, wstring>::const_iterator it2 = it->second.subentries.begin(); it2 != it->second.subentries.end(); ++it2)
		//	{
		//		TVINSERTSTRUCT tv2 = {0};
		//		tv2.hParent = hItem;
		//		tv2.hInsertAfter = TVI_SORT;
		//		tv2.itemex.mask = TVIF_TEXT;
		//		WCHAR * str = new WCHAR[it->first.size()+1];
		//		_tcscpy_s(str, it->first.size()+1, it->first.c_str());
		//		tv.itemex.pszText = str;
		//		TreeView_InsertItem(hTreeControl, &tv2);
		//		delete [] str;
		//	}
		//}
	}

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
	HDWP hdwp = BeginDeferWindowPos(3);
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

		GetWindowRect(GetDlgItem(*this, IDC_LOGINFO), &treelist);
		treelist.left = point2.x + 2;
		MapWindowPoints(NULL, *this, (LPPOINT)&treelist, 2);
		DeferWindowPos(hdwp, GetDlgItem(*this, IDC_LOGINFO), NULL, 
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
