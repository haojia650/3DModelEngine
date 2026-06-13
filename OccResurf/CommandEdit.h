#pragma once

#include <vector>

constexpr UINT WM_OCC_COMMAND_ENTERED = WM_APP + 101;

class CCommandEdit : public CEdit
{
public:
	CCommandEdit() noexcept;

	void SetCompletionWords(const std::vector<CString>& words);
	void SetCommandHistory(const std::vector<CString>& history);
	void ClearCommandHistory();
	const std::vector<CString>& GetCommandHistory() const { return m_history; }
	CString ConsumeEnteredCommand();

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg UINT OnGetDlgCode();
	DECLARE_MESSAGE_MAP()

private:
	void SubmitCommand();
	void RecallHistory(int direction);
	void CompleteCurrentText();
	void AddToHistory(const CString& command);

	std::vector<CString> m_history;
	std::vector<CString> m_completionWords;
	int m_historyIndex;
	CString m_enteredCommand;
};
