
// SessionSchedulerGUIDlg.h: файл заголовка
//

#pragma once
#include <string>

// Диалоговое окно CSessionSchedulerGUIDlg
class CSessionSchedulerGUIDlg : public CDialogEx
{
// Создание
public:
	CSessionSchedulerGUIDlg(CWnd* pParent = nullptr);	// стандартный конструктор

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SESSIONSCHEDULERGUI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// поддержка DDX/DDV


// Реализация
protected:
	HICON m_hIcon;

	// Созданные функции схемы сообщений
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnInput();
	afx_msg void OnBnClickedBtnOutput();
	afx_msg void OnBnClickedBtnRun();

private:
	CString m_inputPath;
	CString m_outputPath;

	void AppendLog(const CString& text);
	void RunScheduling(const std::string& filePath, const std::string& outputFilePath);

public:
	afx_msg LRESULT OnLogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWorkDone(WPARAM wParam, LPARAM lParam);
};
