
#pragma once

#include "CommandEdit.h"

/////////////////////////////////////////////////////////////////////////////
// COutputList 窗口

class COutputList : public CListBox
{
// 构造
public:
	COutputList() noexcept;

// 实现
public:
	virtual ~COutputList();

protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();

	DECLARE_MESSAGE_MAP()
};

class COutputWnd : public CDockablePane
{
// 构造
public:
	COutputWnd() noexcept;

	void UpdateFonts();
	void AppendMessage(const CString& message, bool isError = false);
	void ExecuteCommandLine(const CString& command);
	void SetCommandHistory(const CStringArray& history);
	void GetCommandHistory(CStringArray& history) const;

// 特性
protected:
	CMFCTabCtrl	m_wndTabs;

	CStatic m_wndCommandLabel;
	COutputList m_wndOutputBuild;
	COutputList m_wndOutputDebug;
	COutputList m_wndOutputFind;
	CListBox m_wndCommandHistory;
	CCommandEdit m_wndCommandEdit;
	CFont m_commandFont;

protected:
	void FillBuildWindow();
	void FillDebugWindow();
	void FillFindWindow();
	void FillCommandWindow();
	void LayoutCommandControls();
	bool ParsePositiveDouble(const CString& token, double& value) const;

	void AdjustHorzScroll(CListBox& wndListBox);

// 实现
public:
	virtual ~COutputWnd();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnCommandEntered(WPARAM wp, LPARAM lp);

	DECLARE_MESSAGE_MAP()
};

