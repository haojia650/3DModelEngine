#include "pch.h"
#include "CommandEdit.h"

#include <algorithm>

CCommandEdit::CCommandEdit() noexcept
	: m_historyIndex(-1)
{
	m_completionWords =
	{
		_T("box"),
		_T("sphere"),
		_T("cylinder"),
		_T("move"),
		_T("rotate"),
		_T("delete"),
		_T("undo"),
		_T("redo")
	};
}

BEGIN_MESSAGE_MAP(CCommandEdit, CEdit)
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

void CCommandEdit::SetCompletionWords(const std::vector<CString>& words)
{
	m_completionWords = words;
}

void CCommandEdit::SetCommandHistory(const std::vector<CString>& history)
{
	m_history = history;
	if (m_history.size() > 50)
	{
		m_history.resize(50);
	}
	m_historyIndex = -1;
}

void CCommandEdit::ClearCommandHistory()
{
	m_history.clear();
	m_historyIndex = -1;
}

CString CCommandEdit::ConsumeEnteredCommand()
{
	CString command = m_enteredCommand;
	m_enteredCommand.Empty();
	return command;
}

BOOL CCommandEdit::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg != nullptr && pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			SubmitCommand();
			return TRUE;
		case VK_UP:
			RecallHistory(1);
			return TRUE;
		case VK_DOWN:
			RecallHistory(-1);
			return TRUE;
		case VK_TAB:
			CompleteCurrentText();
			return TRUE;
		default:
			break;
		}
	}

	return CEdit::PreTranslateMessage(pMsg);
}

UINT CCommandEdit::OnGetDlgCode()
{
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

void CCommandEdit::SubmitCommand()
{
	CString command;
	GetWindowText(command);
	command.Trim();
	if (command.IsEmpty())
	{
		SetWindowText(_T(""));
		return;
	}

	AddToHistory(command);
	m_enteredCommand = command;
	SetWindowText(_T(""));
	m_historyIndex = -1;

	CWnd* owner = GetOwner();
	if (owner == nullptr)
	{
		owner = GetParent();
	}
	if (owner != nullptr)
	{
		owner->PostMessage(WM_OCC_COMMAND_ENTERED, GetDlgCtrlID(), reinterpret_cast<LPARAM>(GetSafeHwnd()));
	}
}

void CCommandEdit::RecallHistory(int direction)
{
	if (m_history.empty())
	{
		SetWindowText(_T(""));
		m_historyIndex = -1;
		return;
	}

	if (direction > 0)
	{
		if (m_historyIndex < static_cast<int>(m_history.size()) - 1)
		{
			++m_historyIndex;
		}
	}
	else
	{
		if (m_historyIndex > -1)
		{
			--m_historyIndex;
		}
	}

	if (m_historyIndex >= 0 && m_historyIndex < static_cast<int>(m_history.size()))
	{
		SetWindowText(m_history[static_cast<size_t>(m_historyIndex)]);
		SetSel(0, -1);
	}
	else
	{
		SetWindowText(_T(""));
	}
}

void CCommandEdit::CompleteCurrentText()
{
	CString text;
	GetWindowText(text);
	text.TrimLeft();
	if (text.IsEmpty())
	{
		return;
	}

	CString lowerText(text);
	lowerText.MakeLower();
	for (const auto& word : m_completionWords)
	{
		CString lowerWord(word);
		lowerWord.MakeLower();
		if (lowerWord.Left(lowerText.GetLength()) == lowerText)
		{
			SetWindowText(word);
			SetSel(word.GetLength(), word.GetLength());
			return;
		}
	}
}

void CCommandEdit::AddToHistory(const CString& command)
{
	auto duplicate = std::find_if(m_history.begin(), m_history.end(),
		[&command](const CString& item) { return item.CompareNoCase(command) == 0; });
	if (duplicate != m_history.end())
	{
		m_history.erase(duplicate);
	}

	m_history.insert(m_history.begin(), command);
	if (m_history.size() > 50)
	{
		m_history.resize(50);
	}
}
