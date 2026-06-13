
#include "pch.h"
#include "framework.h"

#include "OutputWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "OccResurfDoc.h"
#include "OccResurfView.h"

#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace
{
constexpr UINT kCommandHistoryId = 1001;
constexpr UINT kCommandEditId = 1002;
}

/////////////////////////////////////////////////////////////////////////////
// COutputBar

COutputWnd::COutputWnd() noexcept
{
}

COutputWnd::~COutputWnd()
{
}

BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_MESSAGE(WM_OCC_COMMAND_ENTERED, &COutputWnd::OnCommandEntered)
END_MESSAGE_MAP()

int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// 创建选项卡窗口: 
	if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, 1, CMFCTabCtrl::LOCATION_TOP))
	{
		TRACE0("未能创建输出选项卡窗口\n");
		return -1;      // 未能创建
	}
	m_wndTabs.EnableTabSwap(FALSE);

	// 创建输出窗格: 
	const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

	if (!m_wndOutputBuild.Create(dwStyle, rectDummy, &m_wndTabs, 2) ||
		!m_wndOutputDebug.Create(dwStyle, rectDummy, &m_wndTabs, 3) ||
		!m_wndOutputFind.Create(dwStyle, rectDummy, &m_wndTabs, 4) ||
		!m_wndCommandHistory.Create(dwStyle, rectDummy, &m_wndTabs, kCommandHistoryId))
	{
		TRACE0("未能创建输出窗口\n");
		return -1;      // 未能创建
	}

	if (!m_wndCommandLabel.Create(_T("命令:"), WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE, rectDummy, this))
	{
		TRACE0("未能创建命令行标签\n");
		return -1;
	}

	if (!m_wndCommandEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, rectDummy, this, kCommandEditId))
	{
		TRACE0("未能创建命令行输入框\n");
		return -1;
	}
	m_wndCommandEdit.SetCueBanner(_T("输入命令，例如 box 20 10 8，然后按 Enter"), TRUE);

	UpdateFonts();

	CString strTabName;
	BOOL bNameValid;

	// 将列表窗口附加到选项卡: 
	m_wndTabs.AddTab(&m_wndCommandHistory, _T("命令"), (UINT)0);
	bNameValid = strTabName.LoadString(IDS_BUILD_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputBuild, strTabName, (UINT)1);
	bNameValid = strTabName.LoadString(IDS_DEBUG_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputDebug, strTabName, (UINT)2);
	bNameValid = strTabName.LoadString(IDS_FIND_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputFind, strTabName, (UINT)3);
	m_wndTabs.SetActiveTab(0);

	// 使用一些虚拟文本填写输出选项卡(无需复杂数据)
	FillBuildWindow();
	FillDebugWindow();
	FillFindWindow();
	FillCommandWindow();

	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	LayoutCommandControls();
}

void COutputWnd::AdjustHorzScroll(CListBox& wndListBox)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(m_commandFont.GetSafeHandle() != nullptr ? &m_commandFont : &afxGlobalData.fontRegular);

	int cxExtentMax = 0;

	for (int i = 0; i < wndListBox.GetCount(); i ++)
	{
		CString strItem;
		wndListBox.GetText(i, strItem);

		cxExtentMax = max(cxExtentMax, (int)dc.GetTextExtent(strItem).cx);
	}

	wndListBox.SetHorizontalExtent(cxExtentMax);
	dc.SelectObject(pOldFont);
}

void COutputWnd::FillBuildWindow()
{
	m_wndOutputBuild.AddString(_T("生成输出正显示在此处。"));
	m_wndOutputBuild.AddString(_T("输出正显示在列表视图的行中"));
	m_wndOutputBuild.AddString(_T("但您可以根据需要更改其显示方式..."));
}

void COutputWnd::FillDebugWindow()
{
	m_wndOutputDebug.AddString(_T("调试输出正显示在此处。"));
	m_wndOutputDebug.AddString(_T("输出正显示在列表视图的行中"));
	m_wndOutputDebug.AddString(_T("但您可以根据需要更改其显示方式..."));
}

void COutputWnd::FillFindWindow()
{
	m_wndOutputFind.AddString(_T("查找输出正显示在此处。"));
	m_wndOutputFind.AddString(_T("输出正显示在列表视图的行中"));
	m_wndOutputFind.AddString(_T("但您可以根据需要更改其显示方式..."));
}

void COutputWnd::FillCommandWindow()
{
	AppendMessage(_T("命令行已就绪。示例: box 20 10 8, sphere 12, cylinder 5 30, delete, undo, redo"));
	AppendMessage(_T("格式: box 长 宽 高 | sphere 半径 | cylinder 半径 高度"));
}

void COutputWnd::UpdateFonts()
{
	if (m_commandFont.GetSafeHandle() != nullptr)
	{
		m_commandFont.DeleteObject();
	}

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);
	CDC* dc = GetDC();
	const int dpiY = dc != nullptr ? dc->GetDeviceCaps(LOGPIXELSY) : 96;
	if (dc != nullptr)
	{
		ReleaseDC(dc);
	}
	lf.lfHeight = MulDiv(11, dpiY, -72);
	lf.lfWeight = FW_NORMAL;
	m_commandFont.CreateFontIndirect(&lf);

	CFont* font = m_commandFont.GetSafeHandle() != nullptr ? &m_commandFont : &afxGlobalData.fontRegular;
	m_wndOutputBuild.SetFont(font);
	m_wndOutputDebug.SetFont(font);
	m_wndOutputFind.SetFont(font);
	m_wndCommandHistory.SetFont(font);
	m_wndCommandLabel.SetFont(font);
	m_wndCommandEdit.SetFont(font);
}

void COutputWnd::AppendMessage(const CString& message, bool isError)
{
	CString line = message;
	if (isError)
	{
		line = _T("ERROR: ") + line;
	}
	const int index = m_wndCommandHistory.AddString(line);
	m_wndCommandHistory.SetCurSel(index);
	AdjustHorzScroll(m_wndCommandHistory);
}

void COutputWnd::SetCommandHistory(const CStringArray& history)
{
	std::vector<CString> items;
	items.reserve(static_cast<size_t>(history.GetCount()));
	for (INT_PTR i = 0; i < history.GetCount(); ++i)
	{
		items.push_back(history.GetAt(i));
	}
	m_wndCommandEdit.SetCommandHistory(items);
}

void COutputWnd::GetCommandHistory(CStringArray& history) const
{
	history.RemoveAll();
	for (const auto& item : m_wndCommandEdit.GetCommandHistory())
	{
		history.Add(item);
	}
}

void COutputWnd::ExecuteCommandLine(const CString& command)
{
	CString trimmed = command;
	trimmed.Trim();
	if (trimmed.IsEmpty())
	{
		return;
	}

	AppendMessage(_T("> ") + trimmed);

	std::vector<CString> tokens;
	int current = 0;
	CString token = trimmed.Tokenize(_T(" \t"), current);
	while (!token.IsEmpty())
	{
		tokens.push_back(token);
		token = trimmed.Tokenize(_T(" \t"), current);
	}
	if (tokens.empty())
	{
		return;
	}

	CString verb = tokens[0];
	verb.MakeLower();
	int selectedObjectId = 0;

	CMainFrame* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	COccResurfDoc* doc = mainFrame != nullptr ? mainFrame->GetActiveOccDocument() : nullptr;
	COccResurfView* view = mainFrame != nullptr ? mainFrame->GetActiveOccView() : nullptr;
	if (doc == nullptr)
	{
		AppendMessage(_T("No active document."), true);
		return;
	}

	if (verb == _T("box"))
	{
		if (tokens.size() != 4)
		{
			AppendMessage(_T("Usage: box length width height"), true);
			return;
		}
		double length = 0.0, width = 0.0, height = 0.0;
		if (!ParsePositiveDouble(tokens[1], length) || !ParsePositiveDouble(tokens[2], width) || !ParsePositiveDouble(tokens[3], height))
		{
			AppendMessage(_T("Box dimensions must be positive numbers."), true);
			return;
		}
		if (view != nullptr)
		{
			view->RecordUndoSnapshot();
		}
		const int id = doc->AddBoxObject(length, width, height);
		selectedObjectId = id;
		CString msg;
		msg.Format(_T("Created Box_%d"), id);
		AppendMessage(msg);
	}
	else if (verb == _T("sphere"))
	{
		if (tokens.size() != 2)
		{
			AppendMessage(_T("Usage: sphere radius"), true);
			return;
		}
		double radius = 0.0;
		if (!ParsePositiveDouble(tokens[1], radius))
		{
			AppendMessage(_T("Sphere radius must be a positive number."), true);
			return;
		}
		if (view != nullptr)
		{
			view->RecordUndoSnapshot();
		}
		const int id = doc->AddSphereObject(radius);
		selectedObjectId = id;
		CString msg;
		msg.Format(_T("Created Sphere_%d"), id);
		AppendMessage(msg);
	}
	else if (verb == _T("cylinder"))
	{
		if (tokens.size() != 3)
		{
			AppendMessage(_T("Usage: cylinder radius height"), true);
			return;
		}
		double radius = 0.0, height = 0.0;
		if (!ParsePositiveDouble(tokens[1], radius) || !ParsePositiveDouble(tokens[2], height))
		{
			AppendMessage(_T("Cylinder parameters must be positive numbers."), true);
			return;
		}
		if (view != nullptr)
		{
			view->RecordUndoSnapshot();
		}
		const int id = doc->AddCylinderObject(radius, height);
		selectedObjectId = id;
		CString msg;
		msg.Format(_T("Created Cylinder_%d"), id);
		AppendMessage(msg);
	}
	else if (verb == _T("delete"))
	{
		if (view != nullptr)
		{
			view->RecordUndoSnapshot();
		}
		const int deleted = doc->DeleteSelectedModelObjects();
		CString msg;
		msg.Format(_T("Deleted %d selected object(s)."), deleted);
		AppendMessage(msg);
	}
	else if (verb == _T("undo"))
	{
		if (view != nullptr && view->UndoLastAction())
		{
			AppendMessage(_T("Undo complete."));
		}
		else
		{
			AppendMessage(_T("Nothing to undo."), true);
		}
	}
	else if (verb == _T("redo"))
	{
		if (view != nullptr && view->RedoLastAction())
		{
			AppendMessage(_T("Redo complete."));
		}
		else
		{
			AppendMessage(_T("Nothing to redo."), true);
		}
	}
	else
	{
		AppendMessage(_T("Unknown command. Try box, sphere, cylinder, delete, undo, redo."), true);
		return;
	}

	if (view != nullptr)
	{
		view->FitAll();
		view->Invalidate(FALSE);
	}
	if (mainFrame != nullptr)
	{
		mainFrame->RefreshObjectPanels(selectedObjectId);
	}
}

void COutputWnd::LayoutCommandControls()
{
	if (GetSafeHwnd() == nullptr)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);
	const int margin = 6;
	const int labelWidth = 56;
	const int editHeight = 28;
	const int commandTop = rectClient.top + margin;
	const int tabsTop = commandTop + editHeight + margin;
	const int tabsHeight = max(0, rectClient.bottom - tabsTop - margin);

	m_wndCommandLabel.SetWindowPos(nullptr,
		rectClient.left + margin,
		commandTop,
		labelWidth,
		editHeight,
		SWP_NOACTIVATE | SWP_NOZORDER);

	m_wndCommandEdit.SetWindowPos(nullptr,
		rectClient.left + margin + labelWidth + margin,
		commandTop,
		max(0, rectClient.Width() - labelWidth - margin * 3),
		editHeight,
		SWP_NOACTIVATE | SWP_NOZORDER);

	m_wndTabs.SetWindowPos(nullptr,
		rectClient.left + margin,
		tabsTop,
		max(0, rectClient.Width() - margin * 2),
		tabsHeight,
		SWP_NOACTIVATE | SWP_NOZORDER);
}

bool COutputWnd::ParsePositiveDouble(const CString& token, double& value) const
{
	TCHAR* end = nullptr;
	value = _tcstod(token, &end);
	return end != token.GetString() && end != nullptr && *end == _T('\0') && value > 0.0;
}

LRESULT COutputWnd::OnCommandEntered(WPARAM, LPARAM)
{
	ExecuteCommandLine(m_wndCommandEdit.ConsumeEnteredCommand());
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// COutputList1

COutputList::COutputList() noexcept
{
}

COutputList::~COutputList()
{
}

BEGIN_MESSAGE_MAP(COutputList, CListBox)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_VIEW_OUTPUTWND, OnViewOutput)
	ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// COutputList 消息处理程序

void COutputList::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_OUTPUT_POPUP);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}

void COutputList::OnEditCopy()
{
	MessageBox(_T("复制输出"));
}

void COutputList::OnEditClear()
{
	MessageBox(_T("清除输出"));
}

void COutputList::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame != nullptr && pParentBar != nullptr)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();

	}
}
