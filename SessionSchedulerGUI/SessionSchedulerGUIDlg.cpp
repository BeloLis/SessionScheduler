// SessionSchedulerGUIDlg.cpp: файл реализации
#include "pch.h"
#include "framework.h"
#include "SessionSchedulerGUI.h"
#include "SessionSchedulerGUIDlg.h"
#include "afxdialogex.h"
#include "ExcelReader.h"
#include "Scheduler.h"
#include <thread>
#include <atlconv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Диалоговое окно CAboutDlg используется для описания сведений о приложении

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV

// Реализация
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Диалоговое окно CSessionSchedulerGUIDlg



CSessionSchedulerGUIDlg::CSessionSchedulerGUIDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SESSIONSCHEDULERGUI_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSessionSchedulerGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSessionSchedulerGUIDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_INPUT, &CSessionSchedulerGUIDlg::OnBnClickedBtnInput)
	ON_BN_CLICKED(IDC_BTN_OUTPUT, &CSessionSchedulerGUIDlg::OnBnClickedBtnOutput)
	ON_BN_CLICKED(IDC_BTN_RUN, &CSessionSchedulerGUIDlg::OnBnClickedBtnRun)
	ON_MESSAGE(WM_APP + 1, &CSessionSchedulerGUIDlg::OnWorkDone)
	ON_MESSAGE(WM_APP + 2, &CSessionSchedulerGUIDlg::OnLogMessage)
END_MESSAGE_MAP()


// Обработчики сообщений CSessionSchedulerGUIDlg

BOOL CSessionSchedulerGUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Добавление пункта "О программе..." в системное меню.

	// IDM_ABOUTBOX должен быть в пределах системной команды.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Задает значок для этого диалогового окна.  Среда делает это автоматически,
	//  если главное окно приложения не является диалоговым
	SetIcon(m_hIcon, TRUE);			// Крупный значок
	SetIcon(m_hIcon, FALSE);		// Мелкий значок

	// TODO: добавьте дополнительную инициализацию

	return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
}

void CSessionSchedulerGUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CSessionSchedulerGUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // контекст устройства для рисования

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Выравнивание значка по центру клиентского прямоугольника
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Нарисуйте значок
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// Система вызывает эту функцию для получения отображения курсора при перемещении
//  свернутого окна.
HCURSOR CSessionSchedulerGUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSessionSchedulerGUIDlg::AppendLog(const CString& text)
{
	CEdit* logBox = (CEdit*)GetDlgItem(IDC_EDIT_LOG);
	int len = logBox->GetWindowTextLength();
	logBox->SetSel(len, len);
	logBox->ReplaceSel(text + _T("\r\n"));
}

void CSessionSchedulerGUIDlg::OnBnClickedBtnInput()
{
	CFileDialog dlg(TRUE, _T("xlsx"), NULL,
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		_T("Excel файлы (*.xlsx)|*.xlsx|Все файлы (*.*)|*.*||"));

	if (dlg.DoModal() == IDOK) {
		m_inputPath = dlg.GetPathName();
		SetDlgItemText(IDC_EDIT_INPUT_PATH, m_inputPath);
		AppendLog(_T("Выбран входной файл: ") + m_inputPath);
	}
}

void CSessionSchedulerGUIDlg::OnBnClickedBtnOutput()
{
	CFileDialog dlg(FALSE, _T("xlsx"), _T("Output_schedule.xlsx"),
		OFN_OVERWRITEPROMPT,
		_T("Excel файлы (*.xlsx)|*.xlsx|Все файлы (*.*)|*.*||"));

	if (dlg.DoModal() == IDOK) {
		m_outputPath = dlg.GetPathName();
		SetDlgItemText(IDC_EDIT_OUTPUT_PATH, m_outputPath);
		AppendLog(_T("Выходной файл: ") + m_outputPath);
	}
}

void CSessionSchedulerGUIDlg::OnBnClickedBtnRun()
{
	if (m_inputPath.IsEmpty() || m_outputPath.IsEmpty()) {
		AppendLog(_T("Ошибка: выберите входной и выходной файлы."));
		return;
	}

	GetDlgItem(IDC_BTN_RUN)->EnableWindow(FALSE);
	AppendLog(_T("Запуск обработки..."));

	USES_CONVERSION;
	std::string inputPath = CT2A(m_inputPath, CP_UTF8);
	std::string outputPath = CT2A(m_outputPath, CP_UTF8);
	HWND hwndDlg = m_hWnd;

	std::thread([this, inputPath, outputPath, hwndDlg]() {
		RunScheduling(inputPath, outputPath);
		::PostMessage(hwndDlg, WM_APP + 1, 0, 0);
		}).detach();
}

void CSessionSchedulerGUIDlg::RunScheduling(const std::string& filePath, const std::string& outputFilePath)
{
	HWND hwndDlg = m_hWnd;
	auto logToUI = [hwndDlg](const CString& msg) {
		::PostMessage(hwndDlg, WM_APP + 2, (WPARAM)new CString(msg), 0);
		};

	try {
		ExcelReader reader;
		logToUI(_T("Чтение данных из файла..."));

		auto sessionDays = reader.readSessionDays(filePath);
		auto buildings = reader.readBuildings(filePath);
		auto flows = reader.readFlows(filePath);
		auto exams = reader.readExams(filePath);
		auto gradedExams = reader.readGradedExams(filePath);
		auto finalExams = reader.readFinalExams(filePath);
		auto existingSchedule = reader.readExistingSchedule(filePath);

		CString info;
		info.Format(_T("Прочитано: Дней=%d, Зданий=%d, Потоков=%d, З_КР=%d, ЗаО_КП=%d, Экзаменов=%d"),
			(int)sessionDays.size(), (int)buildings.size(), (int)flows.size(),
			(int)exams.size(), (int)gradedExams.size(), (int)finalExams.size());
		logToUI(info);

		Scheduler scheduler(sessionDays, buildings, flows, exams, gradedExams, finalExams, existingSchedule);

		if (scheduler.scheduleExams()) {
			CString success;
			success.Format(_T("Расписание составлено. Успех: %.2f%%"), scheduler.getSuccessRate());
			logToUI(success);
		}
		else {
			logToUI(_T("Не удалось составить расписание полностью."));
		}

		logToUI(_T("Запись результата в файл..."));
		reader.writeAllResults(filePath, outputFilePath, scheduler.getSchedule(), scheduler.getUnscheduledTasks());
		logToUI(_T("Готово! Результат сохранён."));
	}
	catch (const std::exception& e) {
		CString err(e.what());
		logToUI(_T("Ошибка: ") + err);
	}
}

LRESULT CSessionSchedulerGUIDlg::OnLogMessage(WPARAM wParam, LPARAM)
{
	CString* msg = (CString*)wParam;
	AppendLog(*msg);
	delete msg;
	return 0;
}

LRESULT CSessionSchedulerGUIDlg::OnWorkDone(WPARAM, LPARAM)
{
	GetDlgItem(IDC_BTN_RUN)->EnableWindow(TRUE);
	AppendLog(_T("--- Обработка завершена ---"));
	return 0;
}