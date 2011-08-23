// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "CP_Main.h"
#include "MainFrm.h"
#include "afxole.h"
#include "Misc.h"
#include "CopyProperties.h"
#include "InternetUpdate.h"
#include ".\mainfrm.h"
#include "focusdll\focusdll.h"
#include "HyperLink.h"
#include "tinyxml\tinyxml.h"
#include "Path.h"
#include "DittoCopyBuffer.h"
#include "HotKeys.h"
#include "GlobalClips.h"
#include "OptionsSheet.h"

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
    static char THIS_FILE[] = __FILE__;
#endif 

#define WM_ICON_NOTIFY			WM_APP+10
#define MYWM_NOTIFYICON (WM_USER+1)


IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

	BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_FIRST_OPTION, OnFirstOption)
	ON_COMMAND(ID_FIRST_EXIT, OnFirstExit)
	ON_WM_TIMER()
	ON_COMMAND(ID_FIRST_SHOWQUICKPASTE, OnFirstShowquickpaste)
	ON_COMMAND(ID_FIRST_TOGGLECONNECTCV, OnFirstToggleConnectCV)
	ON_UPDATE_COMMAND_UI(ID_FIRST_TOGGLECONNECTCV, OnUpdateFirstToggleConnectCV)
	ON_COMMAND(ID_FIRST_HELP, OnFirstHelp)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HOTKEY, OnHotKey)
	ON_MESSAGE(WM_SHOW_TRAY_ICON, OnShowTrayIcon)
	ON_MESSAGE(WM_CLIPBOARD_COPIED, OnClipboardCopied)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_ADD_TO_DATABASE_FROM_SOCKET, OnAddToDatabaseFromSocket)
	ON_MESSAGE(WM_SEND_RECIEVE_ERROR, OnErrorOnSendRecieve)
	ON_MESSAGE(WM_FOCUS_CHANGED, OnFocusChanged)
	ON_MESSAGE(WM_CUSTOMIZE_TRAY_MENU, OnCustomizeTrayMenu)
	ON_COMMAND(ID_FIRST_IMPORT, OnFirstImport)
	ON_MESSAGE(WM_EDIT_WND_CLOSING, OnEditWndClose)
	ON_WM_DESTROY()
	ON_COMMAND(ID_FIRST_NEWCLIP, OnFirstNewclip)
	ON_MESSAGE(WM_SET_CONNECTED, OnSetConnected)
	ON_MESSAGE(WM_LOAD_ClIP_ON_CLIPBOARD, OnLoadClipOnClipboard)
	ON_MESSAGE(WM_TRAY_MENU_MOUSE_MOVE, OnSystemTrayMouseMove)
	ON_COMMAND(ID_FIRST_GLOBALHOTKEYS, &CMainFrame::OnFirstGlobalhotkeys)
	ON_MESSAGE(WM_GLOBAL_CLIPS_CLOSED, OnGlobalClipsClosed)
	ON_MESSAGE(WM_OPTIONS_CLOSED, OnOptionsClosed)
	ON_MESSAGE(WM_SHOW_OPTIONS, OnShowOptions)
	END_MESSAGE_MAP()

	static UINT indicators[] = 
{
	ID_SEPARATOR,  // status line indicator
	ID_INDICATOR_CAPS, ID_INDICATOR_NUM, ID_INDICATOR_SCRL, 
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    m_pEditFrameWnd = NULL;
    m_keyStateModifiers = 0;
    m_startKeyStateTime = 0;
    m_bMovedSelectionMoveKeyState = false;
    m_keyModifiersTimerCount = 0;
	m_pGlobalClips = NULL;
	m_pOptions = NULL;
}

CMainFrame::~CMainFrame()
{
    if(g_Opt.m_bUseHookDllForFocus)
    {
        Log(_T("Unloading focus dll for tracking focus changes"));
        StopMonitoringFocusChanges();
    }

    CGetSetOptions::SetMainHWND(0);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if(CFrameWnd::OnCreate(lpCreateStruct) ==  - 1)
    {
        return  - 1;
    }

    //Center the main window so message boxes are in the center
    CRect rcScreen;
    GetMonitorRect(0, &rcScreen);
    CPoint cpCenter = rcScreen.CenterPoint();
    MoveWindow(cpCenter.x, cpCenter.x,  - 2,  - 2);

    //Then set the main window to transparent so it's never shown
    //if it is shown then only the task tray icon
    m_Transparency.SetTransparent(m_hWnd, 0, true);

    SetWindowText(_T(""));

    if(g_Opt.m_bUseHookDllForFocus)
    {
        Log(_T("Loading hook dll to track focus changes"));
        MonitorFocusChanges(m_hWnd, WM_FOCUS_CHANGED);
    }
    else
    {
        Log(_T("Setting polling timer to track focus"));
        SetTimer(ACTIVE_WINDOW_TIMER, g_Opt.FocusWndTimerTimeout(), 0);
    }

    SetWindowText(_T("Ditto"));

    HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    m_TrayIcon.Create(NULL, WM_ICON_NOTIFY, _T("Ditto"), hIcon, IDR_MENU, FALSE, _T(""), _T(""), NULL, 20);
    m_TrayIcon.SetSingleClickSelect(TRUE);
    m_TrayIcon.MinimiseToTray(this);
    m_TrayIcon.SetMenuDefaultItem(ID_FIRST_SHOWQUICKPASTE, FALSE);

    //Only if in release
    #ifndef _DEBUG
        {
            //If not showing the icon show it for 40 seconds so they can get to the option
            //in case they can't remember the hot keys or something like that
            if(!(CGetSetOptions::GetShowIconInSysTray()))
            {
                SetTimer(HIDE_ICON_TIMER, 40000, 0);
            }
        }
    #endif 

    SetTimer(CLOSE_WINDOW_TIMER, ONE_HOUR *24, 0);
    SetTimer(REMOVE_OLD_REMOTE_COPIES, ONE_DAY, 0);
    SetTimer(REMOVE_OLD_ENTRIES_TIMER, ONE_MINUTE *15, 0);

    m_ulCopyGap = CGetSetOptions::GetCopyGap();

    theApp.AfterMainCreate();

    m_thread.Start(this);

    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT &cs)
{
    if(cs.hMenu != NULL)
    {
        ::DestroyMenu(cs.hMenu); // delete menu if loaded
        cs.hMenu = NULL; // no menu for this window
    }

    if(!CFrameWnd::PreCreateWindow(cs))
    {
        return FALSE;
    }

    WNDCLASS wc;
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = AfxWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = AfxGetInstanceHandle();
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("Ditto");

    // Create the QPaste window class
    if(!AfxRegisterClass(&wc))
    {
        return FALSE;
    }

    cs.lpszClass = wc.lpszClassName;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
    void CMainFrame::AssertValid()const
    {
        CFrameWnd::AssertValid();
    }

    void CMainFrame::Dump(CDumpContext &dc)const
    {
        CFrameWnd::Dump(dc);
    }

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers




void CMainFrame::OnFirstExit()
{
    this->SendMessage(WM_CLOSE, 0, 0);
}

LRESULT CMainFrame::OnHotKey(WPARAM wParam, LPARAM lParam)
{
    if(theApp.m_pDittoHotKey && wParam == theApp.m_pDittoHotKey->m_Atom)
    {
        //If they still have the shift/ctrl keys down
        if(m_keyStateModifiers != 0 && m_quickPaste.IsWindowVisibleEx())
        {
            Log(_T("On Show Ditto HotKey, key state modifiers are still down, moving selection"));

            if(m_bMovedSelectionMoveKeyState == false)
            {
                Log(_T("Setting flag m_bMovedSelectionMoveKeyState to true, will paste when modifer keys are up"));
            }

            m_quickPaste.MoveSelection(true);
            m_bMovedSelectionMoveKeyState = true;
        }
        else if(g_Opt.m_HideDittoOnHotKeyIfAlreadyShown && m_quickPaste.IsWindowVisibleEx() && g_Opt.GetShowPersistent() == FALSE)
        {
            Log(_T("On Show Ditto HotKey, window is alread visible, hiding window"));
            m_quickPaste.HideQPasteWnd();
        }
        else
        {
            Log(_T("On Show Ditto HotKey, showing window"));

            m_keyModifiersTimerCount = 0;
            m_bMovedSelectionMoveKeyState = false;
            m_startKeyStateTime = GetTickCount();
            m_keyStateModifiers = CAccels::GetKeyStateModifiers();
            SetTimer(KEY_STATE_MODIFIERS, 50, NULL);

			//Before we show our window find the current focused window for paste into
			theApp.m_activeWnd.TrackActiveWnd(NULL);

            m_quickPaste.ShowQPasteWnd(this, false, true, FALSE);
        }

        KillTimer(CLOSE_WINDOW_TIMER);
        SetTimer(CLOSE_WINDOW_TIMER, ONE_HOUR *24, 0);
    }
    else if(theApp.m_pPosOne && wParam == theApp.m_pPosOne->m_Atom)
    {
        Log(_T("Pos 1 hot key"));
        DoFirstTenPositionsPaste(0);
    }
    else if(theApp.m_pPosTwo && wParam == theApp.m_pPosTwo->m_Atom)
    {
        Log(_T("Pos 2 hot key"));
        DoFirstTenPositionsPaste(1);
    }
    else if(theApp.m_pPosThree && wParam == theApp.m_pPosThree->m_Atom)
    {
        Log(_T("Pos 3 hot key"));
        DoFirstTenPositionsPaste(2);
    }
    else if(theApp.m_pPosFour && wParam == theApp.m_pPosFour->m_Atom)
    {
        Log(_T("Pos 4 hot key"));
        DoFirstTenPositionsPaste(3);
    }
    else if(theApp.m_pPosFive && wParam == theApp.m_pPosFive->m_Atom)
    {
        Log(_T("Pos 5 hot key"));
        DoFirstTenPositionsPaste(4);
    }
    else if(theApp.m_pPosSix && wParam == theApp.m_pPosSix->m_Atom)
    {
        Log(_T("Pos 6 hot key"));
        DoFirstTenPositionsPaste(5);
    }
    else if(theApp.m_pPosSeven && wParam == theApp.m_pPosSeven->m_Atom)
    {
        Log(_T("Pos 7 hot key"));
        DoFirstTenPositionsPaste(6);
    }
    else if(theApp.m_pPosEight && wParam == theApp.m_pPosEight->m_Atom)
    {
        Log(_T("Pos 8 hot key"));
        DoFirstTenPositionsPaste(7);
    }
    else if(theApp.m_pPosNine && wParam == theApp.m_pPosNine->m_Atom)
    {
        Log(_T("Pos 9 hot key"));
        DoFirstTenPositionsPaste(8);
    }
    else if(theApp.m_pPosTen && wParam == theApp.m_pPosTen->m_Atom)
    {
        Log(_T("Pos 10 hot key"));
        DoFirstTenPositionsPaste(9);
    }
    else if(theApp.m_pCopyBuffer1 && wParam == theApp.m_pCopyBuffer1->m_Atom)
    {
        Log(_T("Copy buffer 1 hot key"));
        theApp.m_CopyBuffer.StartCopy(0);
    }
    else if(theApp.m_pPasteBuffer1 && wParam == theApp.m_pPasteBuffer1->m_Atom)
    {
        Log(_T("Paste buffer 1 hot key"));
        theApp.m_CopyBuffer.PastCopyBuffer(0);
    }
    else if(theApp.m_pCutBuffer1 && wParam == theApp.m_pCutBuffer1->m_Atom)
    {
        Log(_T("Cut buffer 1 hot key"));
        theApp.m_CopyBuffer.StartCopy(0, true);
    }
    else if(theApp.m_pCopyBuffer2 && wParam == theApp.m_pCopyBuffer2->m_Atom)
    {
        Log(_T("Copy buffer 2 hot key"));
        theApp.m_CopyBuffer.StartCopy(1);
    }
    else if(theApp.m_pPasteBuffer2 && wParam == theApp.m_pPasteBuffer2->m_Atom)
    {
        Log(_T("Paste buffer 2 hot key"));
        theApp.m_CopyBuffer.PastCopyBuffer(1);
    }
    else if(theApp.m_pCutBuffer2 && wParam == theApp.m_pCutBuffer2->m_Atom)
    {
        Log(_T("Cut buffer 2 hot key"));
        theApp.m_CopyBuffer.StartCopy(1, true);
    }
    else if(theApp.m_pCopyBuffer3 && wParam == theApp.m_pCopyBuffer3->m_Atom)
    {
        Log(_T("Copy buffer 3 hot key"));
        theApp.m_CopyBuffer.StartCopy(2);
    }
    else if(theApp.m_pPasteBuffer3 && wParam == theApp.m_pPasteBuffer3->m_Atom)
    {
        Log(_T("Paste buffer 3 hot key"));
        theApp.m_CopyBuffer.PastCopyBuffer(2);
    }
    else if(theApp.m_pCutBuffer3 && wParam == theApp.m_pCutBuffer3->m_Atom)
    {
        Log(_T("Cut buffer 3 hot key"));
        theApp.m_CopyBuffer.StartCopy(2, true);
    }
	else if(theApp.m_pTextOnlyPaste && wParam == theApp.m_pTextOnlyPaste->m_Atom)
	{
		DoTextOnlyPaste();
	}
	else
	{
		for(int i = 0; i < g_HotKeys.GetCount(); i++)
		{
			if(g_HotKeys[i] != NULL && 
				g_HotKeys[i]->m_Atom == wParam && 
				g_HotKeys[i]->m_clipId > 0)
			{
				Log(StrF(_T("Pasting clip from global shortcut, clipId: %d"), g_HotKeys[i]->m_clipId));
				CProcessPaste paste;
				paste.GetClipIDs().Add(g_HotKeys[i]->m_clipId);
				paste.m_bActivateTarget = false;
				paste.m_bSendPaste = true;
				paste.DoPaste();
				theApp.OnPasteCompleted();

				break;
			}
		}
	}

    return TRUE;
}

void CMainFrame::DoTextOnlyPaste()
{
	Log(_T("Text Only paste, saving clipboard to be restored later"));
	m_textOnlyPaste.Save();

	Log(_T("Text Only paste, Add cf_text or cf_unicodetext to clipboard"));
	m_textOnlyPaste.RestoreTextOnly();

	DWORD pasteDelay = g_Opt.GetTextOnlyPasteDelay();
	DWORD restoreDelay = g_Opt.GetTextOnlyRestoreDelay();

	Log(StrF(_T("Text Only paste, delaying %d ms before sending paste"), pasteDelay));

	Sleep(pasteDelay);

	Log(_T("Text Only paste, Sending paste"));
	theApp.m_activeWnd.SendPaste(false);

	Log(StrF(_T("Text Only paste, delaying %d ms before restoring clipboard to original state"), restoreDelay));
	SetTimer(TEXT_ONLY_PASTE, restoreDelay, 0);
}

void CMainFrame::DoFirstTenPositionsPaste(int nPos)
{
    try
    {
        CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT lID FROM Main ")_T("WHERE ((bIsGroup = 1 AND lParentID = -1) OR bIsGroup = 0) ")_T("ORDER BY bIsGroup ASC, clipOrder DESC ")_T("LIMIT 1 OFFSET %d"), nPos);

        if(q.eof() == false)
        {
            if(q.getIntField(_T("bIsGroup")) == FALSE)
            {
                //Don't move these to the top
                BOOL bItWas = g_Opt.m_bUpdateTimeOnPaste;
                g_Opt.m_bUpdateTimeOnPaste = FALSE;

                CProcessPaste paste;
                paste.GetClipIDs().Add(q.getIntField(_T("lID")));
                paste.m_bActivateTarget = false;
                paste.m_bSendPaste = g_Opt.m_bSendPasteOnFirstTenHotKeys ? true : false;
                paste.DoPaste();
                theApp.OnPasteCompleted();

                g_Opt.m_bUpdateTimeOnPaste = bItWas;
            }
        }
    }
    CATCH_SQLITE_EXCEPTION
}

void CMainFrame::DoDittoCopyBufferPaste(int nCopyBuffer)
{
    try
    {
        CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT lID FROM Main WHERE CopyBuffer = %d"), nCopyBuffer);

        if(q.eof() == false)
        {
            //Don't move these to the top
            BOOL bItWas = g_Opt.m_bUpdateTimeOnPaste;
            g_Opt.m_bUpdateTimeOnPaste = FALSE;

            CProcessPaste paste;
            paste.GetClipIDs().Add(q.getIntField(_T("lID")));
            paste.m_bActivateTarget = false;
            paste.DoPaste();
            theApp.OnPasteCompleted();

            g_Opt.m_bUpdateTimeOnPaste = bItWas;
        }
    }
    CATCH_SQLITE_EXCEPTION
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    switch(nIDEvent)
    {
        case HIDE_ICON_TIMER:
            {
                m_TrayIcon.HideIcon();
                KillTimer(nIDEvent);
            }
			break;

        case CLOSE_WINDOW_TIMER:
            {
                m_quickPaste.CloseQPasteWnd();
            }
			break;

        case REMOVE_OLD_ENTRIES_TIMER:
            {
                m_thread.FireDeleteEntries();
            }
			break;

		case REMOVE_OLD_REMOTE_COPIES:
			{
            	m_thread.FireRemoveRemoteFiles();
			}
			break;

        case KEY_STATE_MODIFIERS:
            m_keyModifiersTimerCount++;
            if(m_keyStateModifiers != 0)
            {
                BYTE keyState = CAccels::GetKeyStateModifiers();
                //Have they release the key state modifiers yet(ctrl, shift, alt)
                if((m_keyStateModifiers &keyState) == 0)
                {
                    KillTimer(KEY_STATE_MODIFIERS);
                    long waitTime = (long)(GetTickCount() - m_startKeyStateTime);

                    if(m_bMovedSelectionMoveKeyState || m_keyModifiersTimerCount > g_Opt.GetKeyStateWaitTimerCount())
                    {
                        Log(StrF(_T("Timer KEY_STATE_MODIFIERS timeout count hit(%d), count (%d), time (%d), Move Selection from Modifer (%d) sending paste"), g_Opt.GetKeyStateWaitTimerCount(), m_keyModifiersTimerCount, waitTime, m_bMovedSelectionMoveKeyState));
                        m_quickPaste.OnKeyStateUp();
                    }
                    else
                    {
                        Log(StrF(_T("Timer KEY_STATE_MODIFIERS count NOT hit(%d), count (%d) time (%d)"), g_Opt.GetKeyStateWaitTimerCount(), m_keyModifiersTimerCount, waitTime));
                        m_quickPaste.SetKeyModiferState(false);
                    }

                    m_keyStateModifiers = 0;
                    m_keyModifiersTimerCount = 0;
                    m_bMovedSelectionMoveKeyState = 0;
                }
            }
            else
            {
                KillTimer(KEY_STATE_MODIFIERS);
            }
            break;

        case ACTIVE_WINDOW_TIMER:
			{
				if(m_TrayIcon.Visible())
				{
					theApp.m_activeWnd.TrackActiveWnd(NULL);
				}
			}
			break;

		case FOCUS_CHANGED_TIMER:
			{
				KillTimer(FOCUS_CHANGED_TIMER);
	            //Log(StrF(_T("Focus Timer %d"), m_tempFocusWnd));
	            theApp.m_activeWnd.TrackActiveWnd(m_tempFocusWnd);
			}
            break;

		case TEXT_ONLY_PASTE:
			{
				KillTimer(TEXT_ONLY_PASTE);

				Log(_T("Text Only Paste, restoring original clipboard data"));
				m_textOnlyPaste.Restore();
			}
			break;
    }

    CFrameWnd::OnTimer(nIDEvent);
}

LRESULT CMainFrame::OnShowTrayIcon(WPARAM wParam, LPARAM lParam)
{
    if(lParam)
    {
        if(!m_TrayIcon.Visible())
        {
            KillTimer(HIDE_ICON_TIMER);
            SetTimer(HIDE_ICON_TIMER, 40000, 0);
        }
    }

    if(wParam)
    {
        m_TrayIcon.ShowIcon();
    }
    else
    {
        m_TrayIcon.HideIcon();
    }

    return TRUE;
}

void CMainFrame::OnFirstShowquickpaste()
{
    m_quickPaste.ShowQPasteWnd(this, true, false, FALSE);
}

void CMainFrame::OnFirstToggleConnectCV()
{
    theApp.ToggleConnectCV();
}

void CMainFrame::OnUpdateFirstToggleConnectCV(CCmdUI *pCmdUI)
{
    theApp.UpdateMenuConnectCV(pCmdUI->m_pMenu, ID_FIRST_TOGGLECONNECTCV);
}

LRESULT CMainFrame::OnClipboardCopied(WPARAM wParam, LPARAM lParam)
{
	Log(_T("Start of function OnClipboardCopied, adding clip to thread for processing"));

	CClip *pClip = (CClip*)wParam;
	if(pClip != NULL)
	{
		m_thread.AddClipToSave(pClip);
	} 
    
    Log(_T("End of function OnClipboardCopied"));
    return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG *pMsg)
{
    // target before mouse messages change the focus
    if(theApp.m_bShowingQuickPaste && WM_MOUSEFIRST <= pMsg->message && pMsg->message <= WM_MOUSELAST)
    {
        if(g_Opt.m_bUseHookDllForFocus == false)
        {
            theApp.m_activeWnd.TrackActiveWnd(NULL);
        }
    }

    return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnClose()
{
    CloseAllOpenDialogs();

    if(m_pEditFrameWnd)
    {
        if(m_pEditFrameWnd->CloseAll() == false)
        {
            return ;
        }
    }

    Log(_T("OnClose - before stop MainFrm thread"));
    m_thread.Stop();
    Log(_T("OnClose - after stop MainFrm thread"));

    theApp.BeforeMainClose();

    CFrameWnd::OnClose();
}

bool CMainFrame::CloseAllOpenDialogs()
{
    bool bRet = false;
    DWORD dwordProcessId;
    DWORD dwordChildWindowProcessId;
    GetWindowThreadProcessId(this->m_hWnd, &dwordProcessId);
    ASSERT(dwordProcessId);

    CWnd *pTempWnd = GetDesktopWindow()->GetWindow(GW_CHILD);
    while((pTempWnd = pTempWnd->GetWindow(GW_HWNDNEXT)) != NULL)
    {
        if(pTempWnd->GetSafeHwnd() == NULL)
        {
            break;
        }

        GetWindowThreadProcessId(pTempWnd->GetSafeHwnd(), &dwordChildWindowProcessId);
        if(dwordChildWindowProcessId == dwordProcessId)
        {
            TCHAR szTemp[100];
            GetClassName(pTempWnd->GetSafeHwnd(), szTemp, 100);

            // #32770 is class name for dialogs so don't process the message if it is a dialog
            if(STRCMP(szTemp, _T("#32770")) == 0)
            {
                pTempWnd->SendMessage(WM_CLOSE, 0, 0);
                bRet = true;
            }
        }
    }

    MSG msg;
    while(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return bRet;
}

LRESULT CMainFrame::OnSystemTrayMouseMove(WPARAM wParam, LPARAM lParam)
{
	theApp.m_activeWnd.TrackActiveWnd(NULL);
	return 0;
}

LRESULT CMainFrame::OnLoadClipOnClipboard(WPARAM wParam, LPARAM lParam)
{
	CClip *pClip = (CClip*)wParam;
    if(pClip == NULL)
    {
        LogSendRecieveInfo("---------ERROR OnLoadClipOnClipboard pClip == NULL");
        return FALSE;
    }

    if(pClip)
    {
		CProcessPaste paste;
		paste.m_bSendPaste = false;
		paste.m_bActivateTarget = false;

		LogSendRecieveInfo("---------OnLoadClipOnClipboard - Before PutFormats on clipboard");

		paste.m_pOle->PutFormatOnClipboard(&pClip->m_Formats);
		paste.m_pOle->CacheGlobalData(theApp.m_cfIgnoreClipboard, NewGlobalP("Ignore", sizeof("Ignore")));

		LogSendRecieveInfo("---------OnLoadClipOnClipboard - After PutFormats on clipboard");

		LogSendRecieveInfo(StrF(_T("---------OnLoadClipOnClipboard - Setting clip id: %d on ole clipboard"), pClip->m_id));
		paste.GetClipIDs().Add(pClip->m_id);
		paste.DoPaste();

		LogSendRecieveInfo(StrF(_T("---------OnLoadClipOnClipboard - After paste clip id: %d on ole clipboard"), pClip->m_id));
	}

	delete pClip;

	return TRUE;
}

LRESULT CMainFrame::OnAddToDatabaseFromSocket(WPARAM wParam, LPARAM lParam)
{
    CClipList *pClipList = (CClipList*)wParam;
    if(pClipList == NULL)
    {
        LogSendRecieveInfo("---------OnAddToDatabaseFromSocket - ERROR pClipList == NULL");
        return FALSE;
    }

    BOOL bSetToClipBoard = (BOOL)lParam;
    if(bSetToClipBoard)
    {
        CClip *pClip = pClipList->GetTail();
        if(pClip)
        {
			LogSendRecieveInfo("OnAddToDatabaseFromSocket - Adding clip from socket setting clip to be put on clipboard");
			pClip->m_param1 = TRUE;
		}
    }

	m_thread.AddRemoteClipToSave(pClipList);

	delete pClipList;

    return TRUE;
}

LRESULT CMainFrame::OnErrorOnSendRecieve(WPARAM wParam, LPARAM lParam)
{
    CString csNewText = (TCHAR*)wParam;

    ShowErrorMessage(_T("Ditto - Send/Receive Error"), csNewText);

    return TRUE;
}

CString WndName(HWND hParent)
{
    TCHAR cWindowText[200];

    ::GetWindowText(hParent, cWindowText, 100);

    int nCount = 0;

    while(STRLEN(cWindowText) <= 0)
    {
        hParent = ::GetParent(hParent);
        if(hParent == NULL)
        {
            break;
        }

        ::GetWindowText(hParent, cWindowText, 100);

        nCount++;
        if(nCount > 100)
        {
            Log(_T("GetTargetName reached maximum search depth of 100"));
            break;
        }
    }

    return cWindowText;
}

LRESULT CMainFrame::OnFocusChanged(WPARAM wParam, LPARAM lParam)
{
    if(m_quickPaste.IsWindowVisibleEx())
    {
        HWND focus = (HWND)wParam;
        static DWORD dLastDittoHasFocusTick = 0;

        //Sometimes when we bring ditto up there will come a null focus 
        //rite after that
        if(focus == NULL && (GetTickCount() - dLastDittoHasFocusTick < 500))
        {
            Log(_T("NULL focus within 500 ticks of bringing up ditto"));
            return TRUE;
        }
        else if(focus == NULL)
        {
            Log(_T("NULL focus received"));
        }

        if(theApp.m_activeWnd.DittoHasFocus())
        {
            dLastDittoHasFocusTick = GetTickCount();
        }

        //Log(StrF(_T("OnFocusChanged %d, title %s"), focus, WndName(focus)));
        m_tempFocusWnd = focus;

        KillTimer(FOCUS_CHANGED_TIMER);
        SetTimer(FOCUS_CHANGED_TIMER, g_Opt.FocusChangedDelay(), NULL);
    }

    return TRUE;
}

void CMainFrame::OnFirstHelp()
{
    CString csFile = CGetSetOptions::GetPath(PATH_HELP);
    csFile += "DittoGettingStarted.htm";
    CHyperLink::GotoURL(csFile, SW_SHOW);
}

LRESULT CMainFrame::OnCustomizeTrayMenu(WPARAM wParam, LPARAM lParam)
{
    CMenu *pMenu = (CMenu*)wParam;
    if(pMenu)
    {
        theApp.m_Language.UpdateTrayIconRightClickMenu(pMenu);
    }

    return true;
}

void CMainFrame::ShowErrorMessage(CString csTitle, CString csMessage)
{
    Log(StrF(_T("ShowErrorMessage %s - %s"), csTitle, csMessage));

    CToolTipEx *pErrorWnd = new CToolTipEx;
    pErrorWnd->Create(this);
    pErrorWnd->SetToolTipText(csTitle + "\n\n" + csMessage);

    CPoint pt;
    CRect rcScreen;
    GetMonitorRect(0, &rcScreen);
    pt = rcScreen.BottomRight();

    CRect cr = pErrorWnd->GetBoundsRect();

    pt.x -= max(cr.Width() + 50, 150);
    pt.y -= max(cr.Height() + 50, 150);

    pErrorWnd->Show(pt);
    pErrorWnd->HideWindowInXMilliSeconds(4000);
}

void CMainFrame::OnFirstImport()
{
    theApp.ImportClips(theApp.m_MainhWnd);
}

void CMainFrame::ShowEditWnd(CClipIDs &Ids)
{
    CWaitCursor wait;

    bool bCreatedWindow = false;
    if(m_pEditFrameWnd == NULL)
    {
        m_pEditFrameWnd = new CEditFrameWnd;
        m_pEditFrameWnd->LoadFrame(IDR_MAINFRAME);
        bCreatedWindow = true;
    }
    if(m_pEditFrameWnd)
    {
        m_pEditFrameWnd->EditIds(Ids);
        m_pEditFrameWnd->SetNotifyWnd(m_hWnd);

        if(bCreatedWindow)
        {
            CSize sz;
            CPoint pt;
            CGetSetOptions::GetEditWndSize(sz);
            CGetSetOptions::GetEditWndPoint(pt);
            CRect cr(pt, sz);
            EnsureWindowVisible(&cr);
            m_pEditFrameWnd->MoveWindow(cr);
        }

        m_pEditFrameWnd->ShowWindow(SW_SHOW);
        m_pEditFrameWnd->SetForegroundWindow();
        m_pEditFrameWnd->SetFocus();
    }
}

LRESULT CMainFrame::OnEditWndClose(WPARAM wParam, LPARAM lParam)
{
    m_pEditFrameWnd = NULL;
    return TRUE;
}

LRESULT CMainFrame::OnSetConnected(WPARAM wParam, LPARAM lParam)
{
    if(wParam)
    {
        theApp.SetConnectCV(true);
    }
    else if(lParam)
    {
        theApp.SetConnectCV(false);
    }

    return TRUE;
}

void CMainFrame::OnDestroy()
{
    CFrameWnd::OnDestroy();

    if(m_pEditFrameWnd)
    {
        m_pEditFrameWnd->DestroyWindow();
    }
}

void CMainFrame::OnFirstNewclip()
{
    CClipIDs IDs;
    IDs.Add( - 1);
    theApp.EditItems(IDs, true);
}

void CMainFrame::OnFirstOption()
{
	if(m_pOptions != NULL)
	{
		::SetForegroundWindow(m_pOptions->m_hWnd);
	}
	else
	{
		m_pOptions = new COptionsSheet(_T(""));

		if(m_pOptions != NULL)
		{
			((COptionsSheet*)m_pOptions)->SetNotifyWnd(m_hWnd);
			m_pOptions->Create();
			m_pOptions->ShowWindow(SW_SHOW);
		}
	}
}

void CMainFrame::OnFirstGlobalhotkeys()
{
	if(m_pGlobalClips != NULL)
	{
		::SetForegroundWindow(m_pGlobalClips->m_hWnd);
	}
	else
	{
		m_pGlobalClips = new GlobalClips();

		if(m_pGlobalClips != NULL)
		{
			((GlobalClips*)m_pGlobalClips)->SetNotifyWnd(m_hWnd);
			m_pGlobalClips->Create(IDD_GLOBAL_CLIPS, NULL);
			m_pGlobalClips->ShowWindow(SW_SHOW);
		}
	}
}

LRESULT CMainFrame::OnShowOptions(WPARAM wParam, LPARAM lParam)
{
	OnFirstOption();
	return 0;
}

LRESULT CMainFrame::OnOptionsClosed(WPARAM wParam, LPARAM lParam)
{
	delete m_pOptions;
	m_pOptions = NULL;

	return 0;
}

LRESULT CMainFrame::OnGlobalClipsClosed(WPARAM wParam, LPARAM lParam)
{
	delete m_pGlobalClips;
	m_pGlobalClips = NULL;

	return 0;
}