#include "StdAfx.h"
#include <fstream>
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")

#include "HiddenWindow.h"
#include "resource.h"
#include "MainDlg.h"
#include "SVN.h"
#include "Callback.h"
#include "AppUtils.h"
#include "TempFile.h"
#include "StatusBarMsgWnd.h"

#include <boost/regex.hpp>
using namespace boost;

// for Vista
#define MSGFLT_ADD 1


extern HINSTANCE hInst;

DWORD WINAPI MonitorThread(LPVOID lpParam);

CHiddenWindow::PFNCHANGEWINDOWMESSAGEFILTER CHiddenWindow::m_pChangeWindowMessageFilter = NULL;

CHiddenWindow::CHiddenWindow(HINSTANCE hInst, const WNDCLASSEX* wcx /* = NULL*/) 
	: CWindow(hInst, wcx)
	, m_ThreadRunning(0)
	, m_bRun(true)
	, m_hMonitorThread(NULL)
	, m_bMainDlgShown(false)
	, m_bMainDlgRemovedItems(false)
	, regShowTaskbarIcon(_T("Software\\CommitMonitor\\TaskBarIcon"), FALSE)

{
	m_hIconNew = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOTIFYNEW));
	m_hIconNormal = LoadIcon(hInst, MAKEINTRESOURCE(IDI_COMMITMONITOR));
	ZeroMemory(&m_SystemTray, sizeof(m_SystemTray));
}

CHiddenWindow::~CHiddenWindow(void)
{
	DestroyIcon(m_hIconNew);
	DestroyIcon(m_hIconNormal);

	Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
}

bool CHiddenWindow::RegisterAndCreateWindow()
{
	WNDCLASSEX wcx; 

	// Fill in the window class structure with default parameters 
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = CWindow::stWinMsgHandler;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hResource;
	wcx.hCursor = LoadCursor(NULL, IDC_SIZEWE);
	wcx.lpszClassName = ResString(hResource, IDS_APP_TITLE);
	wcx.hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_COMMITMONITOR));
	wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wcx.lpszMenuName = MAKEINTRESOURCE(IDC_COMMITMONITOR);
	wcx.hIconSm	= LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_COMMITMONITOR));
	if (RegisterWindow(&wcx))
	{
		if (Create(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL))
		{
			COMMITMONITOR_SHOWDLGMSG = RegisterWindowMessage(_T("CommitMonitor_ShowDlgMsg"));
			WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));
			// On Vista, the message TasbarCreated may be blocked by the message filter.
			// We try to change the filter here to get this message through. If even that
			// fails, then we can't do much about it and the task bar icon won't show up again.
			HMODULE hLib = LoadLibrary(_T("user32.dll"));
			if (hLib)
			{
				m_pChangeWindowMessageFilter = (CHiddenWindow::PFNCHANGEWINDOWMESSAGEFILTER)GetProcAddress(hLib, "ChangeWindowMessageFilter");
				if (m_pChangeWindowMessageFilter)
				{
					(*m_pChangeWindowMessageFilter)(WM_TASKBARCREATED, MSGFLT_ADD);
				}
			}
			ShowWindow(m_hwnd, SW_HIDE);
			ShowTrayIcon(false);
			m_UrlInfos.Load();
			return true;
		}
	}
	return false;
}

INT_PTR CHiddenWindow::ShowDialog()
{
	return ::SendMessage(*this, COMMITMONITOR_SHOWDLGMSG, 0, 0);
}

LRESULT CHiddenWindow::HandleCustomMessages(HWND /*hwnd*/, UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (uMsg == COMMITMONITOR_SHOWDLGMSG)
	{
		if (m_bMainDlgShown)
			return TRUE;
		m_bMainDlgShown = true;
		m_bMainDlgRemovedItems = false;
		CMainDlg dlg(*this);
		dlg.SetUrlInfos(&m_UrlInfos);
		dlg.DoModal(hInst, IDD_MAINDLG, NULL);
		m_UrlInfos.Save();
		m_bMainDlgShown = false;
		return TRUE;
	}
	else if (uMsg == WM_TASKBARCREATED)
	{
		bool bNew = m_SystemTray.hIcon == m_hIconNew;
		m_SystemTray.hIcon = NULL;
		TRACE(_T("Taskbar created!\n"));
		ShowTrayIcon(bNew);
	}
	return 0L;
}

LRESULT CALLBACK CHiddenWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// the custom messages are not constant, therefore we can't handle them in the
	// switch-case below
	HandleCustomMessages(hwnd, uMsg, wParam, lParam);
	switch (uMsg)
	{
	case WM_CREATE:
		{
			m_hwnd = hwnd;
			// set the timers we use to start the monitoring threads
			::SetTimer(*this, IDT_MONITOR, 10, NULL);
		}
		break;
	case WM_TIMER:
		{
			if (wParam == IDT_MONITOR)
			{
				DoTimer();
			}
		}
		break;
	case COMMITMONITOR_POPUP:
		{
			popupData * pData = (popupData*)lParam;
			if (pData)
			{
				CStatusBarMsgWnd * popup = new CStatusBarMsgWnd(hResource);
				popup->Show(pData->sTitle.c_str(), pData->sText.c_str(), IDI_COMMITMONITOR, *this, 0);
                ShowTrayIcon(true);
			}
		}
		break;
	case COMMITMONITOR_SAVEINFO:
		{
			wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
			m_UrlInfos.Save(urlfile.c_str());
			return TRUE;
		}
		break;
	case COMMITMONITOR_REMOVEDURL:
		{
			m_bMainDlgRemovedItems = true;
		}
		break;
	case COMMITMONITOR_CHANGEDINFO: 		    
		ShowTrayIcon(!!wParam);
		return TRUE;
	case COMMITMONITOR_TASKBARCALLBACK:
		{
			switch (lParam)
			{
			case WM_MOUSEMOVE:
				{
					// update the tool tip data
				}
				break;
			case WM_LBUTTONDBLCLK:
				{
					// show the main dialog
					ShowDialog();
				}
				break;
			case NIN_KEYSELECT:
			case NIN_SELECT:
			case WM_RBUTTONUP:
			case WM_CONTEXTMENU:
				{
					POINT pt;
					GetCursorPos( &pt );

					HMENU hMenu = ::LoadMenu(hInst, MAKEINTRESOURCE(IDC_COMMITMONITOR));
					hMenu = ::GetSubMenu(hMenu, 0);

					// set the default entry
					MENUITEMINFO iinfo = {0};
					iinfo.cbSize = sizeof(MENUITEMINFO);
					iinfo.fMask = MIIM_STATE;
					GetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);
					iinfo.fState |= MFS_DEFAULT;
					SetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);

					// show the menu
					::SetForegroundWindow(*this);
					int cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY , pt.x, pt.y, NULL, *this, NULL);
					::PostMessage(*this, WM_NULL, 0, 0);

					switch( cmd )
					{
					case IDM_EXIT:
						::PostQuitMessage(0);
						break;
					case ID_POPUP_OPENCOMMITMONITOR:
						ShowDialog();
						break;
					}
				}
				break;
			}
			return TRUE;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		::DestroyWindow(m_hwnd);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
};

void CHiddenWindow::DoTimer()
{
	TRACE(_T("timer fired\n"));
	// Restart the timer with 60 seconds
	::SetTimer(*this, IDT_MONITOR, TIMER_ELAPSE, NULL);

	if (m_ThreadRunning)
		return;
	// go through all url infos and check if
	// we need to refresh them
	if (m_UrlInfos.IsEmpty())
		return;
	bool bStartThread = false;
	__time64_t currenttime = NULL;
	_time64(&currenttime);

	const map<wstring,CUrlInfo> * pInfos = m_UrlInfos.GetReadOnlyData();
	for (map<wstring,CUrlInfo>::const_iterator it = pInfos->begin(); it != pInfos->end(); ++it)
	{
		if ((it->second.lastchecked + (it->second.minutesinterval*60)) < currenttime)
		{
			bStartThread = true;
			break;
		}
	}
	m_UrlInfos.ReleaseReadOnlyData();

	if ((bStartThread)&&(m_ThreadRunning == 0))
	{
		// start the monitoring thread to update the infos
		if (m_hMonitorThread)
		{
			CloseHandle(m_hMonitorThread);
			m_hMonitorThread = NULL;
		}
		DWORD dwThreadId = 0;
		m_hMonitorThread = CreateThread( 
			NULL,              // no security attribute 
			0,                 // default stack size 
			MonitorThread, 
			(LPVOID)this,      // thread parameter 
			0,                 // not suspended 
			&dwThreadId);      // returns thread ID 

		if (m_hMonitorThread == NULL) 
			return;
	}
}

void CHiddenWindow::ShowTrayIcon(bool newCommits)
{
	TRACE(_T("changing tray icon to %s\n"), (newCommits ? _T("\"new commits\"") : _T("\"normal\"")));

	DWORD msg = m_SystemTray.hIcon ? NIM_MODIFY : NIM_ADD;
	if ((!newCommits)&&(regShowTaskbarIcon.read() == FALSE))
	{
		m_SystemTray.hIcon = NULL;
		msg = NIM_DELETE;
	}
	m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
	m_SystemTray.hWnd   = *this;
	m_SystemTray.uID    = 1;
	m_SystemTray.hIcon  = newCommits ? m_hIconNew : m_hIconNormal;
	m_SystemTray.uFlags = NIF_MESSAGE | NIF_ICON;
	m_SystemTray.uCallbackMessage = COMMITMONITOR_TASKBARCALLBACK;
	Shell_NotifyIcon(msg, &m_SystemTray);
	if ((!newCommits)&&(regShowTaskbarIcon.read() == FALSE))
	{
		m_SystemTray.hIcon = NULL;
	}
}

DWORD CHiddenWindow::RunThread()
{
	m_ThreadRunning = TRUE;
	m_bRun = true;
	__time64_t currenttime = NULL;
	_time64(&currenttime);
	bool bNewEntries = false;

	if (::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)!=S_OK)
	{
		return 1;
	}

	// load a copy of the url data
	CUrlInfos urlinfoReadOnly;
	wstring urlfile = CAppUtils::GetAppDataDir() + _T("\\urls");
	if (!PathFileExists(urlfile.c_str()))
	{
		return 0;
	}
	urlinfoReadOnly.Load(urlfile.c_str());
	TRACE(_T("monitor thread started\n"));
	const map<wstring,CUrlInfo> * pUrlInfoReadOnly = urlinfoReadOnly.GetReadOnlyData();
	map<wstring,CUrlInfo>::const_iterator it = pUrlInfoReadOnly->begin();
	for (; (it != pUrlInfoReadOnly->end()) && m_bRun; ++it)
	{
		if ((it->second.lastchecked + (it->second.minutesinterval*60)) < currenttime)
		{
			TRACE(_T("checking %s for updates\n"), it->first.c_str());
			// get the highest revision of the repository
			SVN svn;
			svn.SetAuthInfo(it->second.username, it->second.password);
			svn_revnum_t headrev = svn.GetHEADRevision(it->first);
			if (headrev > it->second.lastcheckedrev)
			{
				TRACE(_T("%s has updates! Last checked revision was %ld, HEAD revision is %ld\n"), it->first.c_str(), it->second.lastcheckedrev, headrev);
				if (svn.GetLog(it->first, headrev, it->second.lastcheckedrev + 1))
				{
					TRACE(_T("log fetched for %s\n"), it->first.c_str());
					
					// only block the object for a short time
					map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
					map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
					if (writeIt != pWrite->end())
					{
						writeIt->second.lastcheckedrev = headrev;
						writeIt->second.lastchecked = currenttime;
					}
					m_UrlInfos.ReleaseWriteData();

					wstring sPopupText;
					for (map<svn_revnum_t,SVNLogEntry>::const_iterator logit = svn.m_logs.begin(); logit != svn.m_logs.end(); ++logit)
					{
						// again, only block for a short time
						map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
						map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
						if (writeIt != pWrite->end())
						{
							writeIt->second.logentries[logit->first] = logit->second;
                            bNewEntries = true;
						}
						m_UrlInfos.ReleaseWriteData();
						TCHAR buf[4096];
						// popup info text
						if (!sPopupText.empty())
							sPopupText += _T(", ");
						sPopupText += logit->second.author;
						if (it->second.fetchdiffs)
						{
							// first, find a name where to store the diff for that revision
							_stprintf_s(buf, 4096, _T("%s_%ld"), it->second.name.c_str(), logit->first);
							wstring diffFileName = CAppUtils::GetAppDataDir();
							diffFileName += _T("/");
							diffFileName += wstring(buf);
							// do we already have that diff?
							if (!PathFileExists(diffFileName.c_str()))
							{
								// get the diff
								if (!svn.Diff(it->first, logit->first - 1, it->first, logit->first, true, true, false, wstring(), false, diffFileName, wstring()))
								{
									TRACE(_T("Diff not fetched for %s, revision %ld because of an error\n"), it->first.c_str(), logit->first);
									DeleteFile(diffFileName.c_str());
								}
								else
									TRACE(_T("Diff fetched for %s, revision %ld\n"), it->first.c_str(), logit->first);
								if (!m_bRun)
									break;
							}
						}
					}
					// prepare notification strings
					TCHAR sTitle[1024] = {0};
					_stprintf_s(sTitle, 1024, _T("%s\nhas %d new commits"), it->second.name.c_str(), svn.m_logs.size());
					popupData data;
					data.sText = sPopupText;
					data.sTitle = wstring(sTitle);
					::SendMessage(*this, COMMITMONITOR_POPUP, 0, (LPARAM)&data);
				}
			}
			else
			{
				// only block the object for a short time
				map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
				map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(it->first);
				if (writeIt != pWrite->end())
				{
					writeIt->second.lastchecked = currenttime;
				}
				m_UrlInfos.ReleaseWriteData();

				// if we can't fetch the HEAD revision, it might be because the URL points to an SVNParentPath
				// instead of pointing to an actual repository.

				// we have to include the authentication in the URL itself
				wstring tempfile = CTempFiles::Instance().GetTempFilePath(true);
				CCallback * callback = new CCallback;
				callback->SetAuthData(it->second.username, it->second.password);
				DeleteFile(tempfile.c_str());
				wstring projName = it->second.name;
				if (URLDownloadToFile(NULL, it->first.c_str(), tempfile.c_str(), 0, callback) == S_OK)
				{
					// we got a web page! But we can't be sure that it's the page from SVNParentPath.
					// Use a regex to parse the website and find out...
					ifstream fs(tempfile.c_str());
					string in;
					if (!fs.bad())
					{
						in.reserve(fs.rdbuf()->in_avail());
						char c;
						while (fs.get(c))
						{
							if (in.capacity() == in.size())
								in.reserve(in.capacity() * 3);
							in.append(1, c);
						}

						// make sure this is a html page from an SVNParentPathList
						// we do this by checking for header titles looking like
						// "<h2>Revision XX: /</h2> - if we find that, it's a html
						// page from inside a repository
                        const char * reTitle = "<\\s*h2\\s*>[^/]+/\\s*<\\s*/\\s*h2\\s*>";
                        // xsl transformed pages don't have an easy way to determine
                        // the inside from outside of a repository.
                        // We therefore check for <index rev="0" to make sure it's either
                        // an empty repository or really an SVNParentPathList
                        const char * reTitle2 = "<\\s*index\\s*rev\\s*=\\s*\"0\"";
                        regex titex = regex(reTitle, regex::normal | regbase::icase);
                        regex titex2 = regex(reTitle2, regex::normal | regbase::icase);
						string::const_iterator start = in.begin();
						string::const_iterator end = in.end();
						match_results<string::const_iterator> fwhat;
						if (regex_search(start, end, fwhat, titex, match_default))
						{
							TRACE(_T("found repository url instead of SVNParentPathList\n"));
							continue;
						}

						const char * re = "<\\s*LI\\s*>\\s*<\\s*A\\s+[^>]*HREF\\s*=\\s*\"([^\"]*)\"\\s*>([^<]+)<\\s*/\\s*A\\s*>\\s*<\\s*/\\s*LI\\s*>";
						const char * re2 = "<\\s*DIR\\s*name\\s*=\\s*\"([^\"]*)\"\\s*HREF\\s*=\\s*\"([^\"]*)\"\\s*/\\s*>";

						regex expression = regex(re, regex::normal | regbase::icase);
						regex expression2 = regex(re2, regex::normal | regbase::icase);
						start = in.begin();
						end = in.end();
						match_results<string::const_iterator> what;
						match_flag_type flags = match_default;
						bool hasNewEntries = false;
						int nCountNewEntries = 0;
						wstring popupText;
						while (regex_search(start, end, what, expression, flags))	
						{
							// what[0] contains the whole string
							// what[1] contains the url part.
							// what[2] contains the name
							wstring url = CUnicodeUtils::StdGetUnicode(string(what[1].first, what[1].second));
							url = it->first + _T("/") + url;
							url = svn.CanonicalizeURL(url);

							map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
							map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(url);
							if ((!m_bMainDlgRemovedItems)&&(writeIt == pWrite->end()))
							{
								// we found a new URL, add it to our list
								CUrlInfo newinfo;
								newinfo.url = url;
								newinfo.name = CUnicodeUtils::StdGetUnicode(string(what[2].first, what[2].second));
								newinfo.name.erase(newinfo.name.find_last_not_of(_T("/ ")) + 1);
								newinfo.username = it->second.username;
								newinfo.password = it->second.password;
								newinfo.fetchdiffs = it->second.fetchdiffs;
								newinfo.minutesinterval = it->second.minutesinterval;
								(*pWrite)[url] = newinfo;
								hasNewEntries = true;
								nCountNewEntries++;
								if (!popupText.empty())
									popupText += _T(", ");
								popupText += newinfo.name;
							}
							m_UrlInfos.ReleaseWriteData();

							// update search position:
							start = what[0].second;		 
							// update flags:
							flags |= match_prev_avail;
							flags |= match_not_bob;
						}
						start = in.begin();
						end = in.end();
                        if (!regex_search(start, end, fwhat, titex2, match_default))
                        {
                            TRACE(_T("found repository url instead of SVNParentPathList\n"));
                            continue;
                        }
						while (regex_search(start, end, what, expression2, flags))	 
						{
							// what[0] contains the whole string
							// what[1] contains the url part.
							// what[2] contains the name
							wstring url = CUnicodeUtils::StdGetUnicode(string(what[1].first, what[1].second));
							url = it->first + _T("/") + url;
							url = svn.CanonicalizeURL(url);

							map<wstring,CUrlInfo> * pWrite = m_UrlInfos.GetWriteData();
							map<wstring,CUrlInfo>::iterator writeIt = pWrite->find(url);
							if ((!m_bMainDlgRemovedItems)&&(writeIt == pWrite->end()))
							{
								// we found a new URL, add it to our list
								CUrlInfo newinfo;
								newinfo.url = url;
								newinfo.name = CUnicodeUtils::StdGetUnicode(string(what[2].first, what[2].second));
								newinfo.name.erase(newinfo.name.find_last_not_of(_T("/ ")) + 1);
								newinfo.username = it->second.username;
								newinfo.password = it->second.password;
								newinfo.fetchdiffs = it->second.fetchdiffs;
								newinfo.minutesinterval = it->second.minutesinterval;
								(*pWrite)[url] = newinfo;
								hasNewEntries = true;
								nCountNewEntries++;
								if (!popupText.empty())
									popupText += _T(", ");
								popupText += newinfo.name;
							}
							m_UrlInfos.ReleaseWriteData();

							// update search position:
							start = what[0].second;		 
							// update flags:
							flags |= match_prev_avail;
							flags |= match_not_bob;
						}
						if (hasNewEntries)
						{
							it = pUrlInfoReadOnly->begin();
							TCHAR popupTitle[1024] = {0};
							_stprintf_s(popupTitle, 1024, _T("%s\nhas %d new projects"), projName.c_str(), nCountNewEntries);
							popupData data;
							data.sText = popupText;
							data.sTitle = wstring(popupTitle);
							::SendMessage(*this, COMMITMONITOR_POPUP, 0, (LPARAM)&data);
                            bNewEntries = true;
						}
					}
					delete callback;
				}
			}
		}
	}
	// save the changed entries
	::PostMessage(*this, COMMITMONITOR_SAVEINFO, (WPARAM)true, (LPARAM)0);
	if (bNewEntries)
		::PostMessage(*this, COMMITMONITOR_CHANGEDINFO, (WPARAM)true, (LPARAM)0);

    urlinfoReadOnly.ReleaseReadOnlyData();
	TRACE(_T("monitor thread ended\n"));
	m_bMainDlgRemovedItems = false;
	m_ThreadRunning = FALSE;

	::CoUninitialize();
	return 0L;
}

DWORD WINAPI MonitorThread(LPVOID lpParam)
{
	CHiddenWindow * pThis = (CHiddenWindow*)lpParam;
	if (pThis)
		return pThis->RunThread();
	return 0L;
}

void CHiddenWindow::StopThread()
{
	m_bRun = false;
	if (m_hMonitorThread)
	{
		WaitForSingleObject(m_hMonitorThread, 2000);
		if (m_ThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_hMonitorThread, (DWORD)-1);
			m_ThreadRunning = false;
		}
		CloseHandle(m_hMonitorThread);
		m_hMonitorThread = NULL;
	}
}