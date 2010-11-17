// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2010 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once
#include <vector>
#include <map>

#include "SCCS.h"

#include <string>

using namespace std;


class ACCUREV : public SCCS
{
public:
    ACCUREV(void);
    ~ACCUREV(void);

    void SetAuthInfo(const std::wstring& username, const std::wstring& password);

    bool Cat(std::wstring sUrl, std::wstring sFile);

    /**
     * returns the info for the \a path.
     * \param path a path or an url
     * \param pegrev the peg revision to use
     * \param revision the revision to get the info for
     * \param recurse if TRUE, then GetNextFileInfo() returns the info also
     * for all children of \a path.
     */
    const SVNInfoData * GetFirstFileInfo(std::wstring path, svn_revnum_t pegrev, svn_revnum_t revision, bool recurse = false);
    size_t GetFileCount() {return (size_t)0;}
    /**
     * Returns the info of the next file in the file list. If no more files are in the list then NULL is returned.
     * See GetFirstFileInfo() for details.
     */
    const SVNInfoData * GetNextFileInfo();

    svn_revnum_t GetHEADRevision(const std::wstring& repo, const std::wstring& url);

    bool GetLog(const std::wstring& repo, const std::wstring& url, svn_revnum_t startrev, svn_revnum_t endrev);
    //map<svn_revnum_t,SVNLogEntry> m_logs;       ///< contains the gathered log information

    bool Diff(const wstring& url1, svn_revnum_t pegrevision, svn_revnum_t revision1,
        svn_revnum_t revision2, bool ignoreancestry, bool nodiffdeleted,
        bool ignorecontenttype,  const wstring& options, bool bAppend,
        const wstring& outputfile, const wstring& errorfile);

    wstring CanonicalizeURL(const wstring& url);
    wstring GetLastErrorMsg();

    /**
     * Sets and clears the progress info which is shown during lengthy operations.
     * \param pProgressDlg the CProgressDlg object to show the progress info on.
     * \param bShowProgressBar set to true if the progress bar should be shown. Only makes
     * sense if the total amount of the progress is known beforehand. Otherwise the
     * progressbar is always "empty".
     */
    void SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar = false);

    /*
    struct SVNProgress
    {
        apr_off_t progress;         ///< operation progress
        apr_off_t total;            ///< operation progress
        apr_off_t overall_total;    ///< total bytes transferred, use SetAndClearProgressInfo() to reset this
        apr_off_t BytesPerSecond;   ///< Speed in bytes per second
        wstring   SpeedString;      ///< String for speed. Either "xxx Bytes/s" or "xxx kBytes/s"
    };

    bool                        m_bCanceled;
    svn_error_t *               Err;            ///< Global error object struct

    */

private:

  bool logParser(const wstring& repo, const wstring& url, const wstring& rawLog);
  bool issueParser(const wstring& rawLog, SVNLogEntry& logEntry);

  // Accurev command line calls
  bool AccuLogin(const wstring& username, const wstring& password);
  bool AccuGetLastPromote(const wstring& repo, const wstring& url, long *pTransactionNo);
  bool AccuGetHistory(const wstring& repo, const wstring& url, long startrev, long endrev, wstring& rawLog);
  bool AccuIssueList(const wstring& repo, const wstring& url, long issueNo, wstring& rawLog);

  size_t ExecuteAccurev(std::wstring Parameters, size_t SecondsToWait, wstring& stdOut, wstring& stdErr);

  void ClearErrors();
  void SetError(const wchar_t *pErrorString);

private:
  svn_error_t errInt;
  const wchar_t *pErrorString;
  
  vector<SVNInfoData>         m_arInfo;       ///< contains all gathered info structs.
  unsigned int                m_pos;          ///< the current position of the vector.
};
