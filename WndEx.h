#if !defined(AFX_WNDEX_H__E14EA019_CE71_469A_AEB4_3D3CB271C531__INCLUDED_)
#define AFX_WNDEX_H__E14EA019_CE71_469A_AEB4_3D3CB271C531__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WndEx.h : header file
//

#define	CAPTION_BORDER			16
#define BORDER					2

#define	SWAP_MIN_MAX			1
#define FORCE_MIN				2
#define FORCE_MAX				3

class CWndEx : public CWnd
{
// Construction
public:
	CWndEx();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndEx)
	public:
	virtual BOOL Create(const CRect& crStart, CWnd* pParentWnd);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

public:
	void	SetResizable(bool bVal)	{ m_bResizable = bVal;	}

	COLORREF		m_CaptionColorLeft;
	COLORREF		m_CaptionColorRight;

	bool	SetCaptionColors( COLORREF left, COLORREF right );
	bool	SetCaptionColorActive( bool bVal );

	void	InvalidateNc();

	void	SetCaptionOn(int nPos, bool bOnstartup = false);
	void	SetAutoHide(BOOL bAutoHide);
	void	MinMaxWindow(long lOption = SWAP_MIN_MAX);
	void	GetWindowRectEx(LPRECT lpRect);

protected:
	CFont			m_TitleFont;
	CFont			m_HorFont;
	bool			m_bResizable;
	CRect			m_crCloseBT;
	CRect			m_crMinimizeBT;
	bool			m_bMouseOverClose;
	bool			m_bMouseDownOnClose;
	bool			m_bMouseDownOnCaption;
	bool			m_bMouseOverMinimize;
	bool			m_bMouseDownOnMinimize;

	long			m_lTopBorder;
	long			m_lRightBorder;
	long			m_lBottomBorder;
	long			m_lLeftBorder;

	CRect			m_crFullSizeWindow;

	bool			m_bMinimized;
	bool			m_bMaxSetTimer;
	
	void			DrawCloseBtn(CWindowDC &dc, COLORREF left = 0);
	void			DrawMinimizeBtn(CWindowDC &dc, COLORREF left = 0);
	void			SetRegion();

// Implementation
public:
	virtual ~CWndEx();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWndEx)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnNcPaint();
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcLButtonUp(UINT nHitTest, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
//	afx_msg BOOL OnNcActivate(BOOL bActive);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WNDEX_H__E14EA019_CE71_469A_AEB4_3D3CB271C531__INCLUDED_)
