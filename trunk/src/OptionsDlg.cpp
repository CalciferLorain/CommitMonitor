#include "StdAfx.h"
#include "Resource.h"
#include "OptionsDlg.h"
#include "Registry.h"
#include <string>

using namespace std;

COptionsDlg::COptionsDlg(HWND hParent)
{
	m_hParent = hParent;
}

COptionsDlg::~COptionsDlg(void)
{
}

LRESULT COptionsDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hwndDlg, IDI_COMMITMONITOR);
			// initialize the controls
			bool bShowTaskbarIcon = !!(DWORD)CRegStdWORD(_T("Software\\CommitMonitor\\TaskBarIcon"), FALSE);
			bool bStartWithWindows = !wstring(CRegStdString(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run\\CommitMonitor"))).empty();
			SendMessage(GetDlgItem(*this, IDC_TASKBAR_ALWAYSON), BM_SETCHECK, bShowTaskbarIcon ? BST_CHECKED : BST_UNCHECKED, NULL);
			SendMessage(GetDlgItem(*this, IDC_AUTOSTART), BM_SETCHECK, bStartWithWindows ? BST_CHECKED : BST_UNCHECKED, NULL);
		}
		return TRUE;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam));
	default:
		return FALSE;
	}
}

LRESULT COptionsDlg::DoCommand(int id)
{
	switch (id)
	{
	case IDOK:
		{
			CRegStdWORD regShowTaskbarIcon = CRegStdWORD(_T("Software\\CommitMonitor\\TaskBarIcon"), FALSE);
			CRegStdString regStartWithWindows = CRegStdString(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run\\CommitMonitor"));
			bool bShowTaskbarIcon = !!SendMessage(GetDlgItem(*this, IDC_TASKBAR_ALWAYSON), BM_GETCHECK, 0, NULL);
			bool bStartWithWindows = !!SendMessage(GetDlgItem(*this, IDC_AUTOSTART), BM_GETCHECK, 0, NULL);
			regShowTaskbarIcon = bShowTaskbarIcon;
			::SendMessage(m_hHiddenWnd, COMMITMONITOR_CHANGEDINFO, 0, 0);
			if (bStartWithWindows)
			{
				TCHAR buf[MAX_PATH*4];
				GetModuleFileName(NULL, buf, MAX_PATH*4);
				regStartWithWindows = wstring(buf);
			}
			else
				regStartWithWindows.removeValue();
		}
		// fall through
	case IDCANCEL:
		EndDialog(*this, id);
		break;
	}
	return 1;
}

