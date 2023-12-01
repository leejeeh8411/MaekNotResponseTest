
// MFCApplication1Dlg.cpp : ���� ����
//

#include "stdafx.h"
#include "MFCApplication1.h"
#include "MFCApplication1Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


UINT ThreadReadIO(LPVOID wParam)
{
	CMFCApplication1Dlg* main = (CMFCApplication1Dlg*)wParam;

	int nModule = 1;
	DWORD dVal = 0;
	while (true) {
		
		AxdiReadInportBit(nModule, 0, &dVal);
		if (main->m_bOldTrgVal != (bool)dVal) {
			if (dVal) {
				DWORD uValue;
				AxdiReadInportByte(nModule, 0, &uValue);
				DWORD DCellNo = uValue >> 0x01;
				main->m_nCellNo = (int)DCellNo;
				/*main->m_gLog->info(_T("ReadTrg CellNo:{},beforeTrg:{},currentTrg:{}"), 
					main->m_nCellNo, 
					main->m_bOldTrgVal,
					(bool)dVal
				);*/
				main->m_mutex.lock();
				main->m_queue.emplace((int)DCellNo);
				main->m_mutex.unlock();
				main->m_gLog->info(_T("ReadTrg CellNo:{}"), main->m_nCellNo);
				main->m_bEventWriteInkIO = true;
				main->m_bOldTrgVal = (bool)dVal;
			}
			main->m_bOldTrgVal = (bool)dVal;
		}

		Sleep(1);
	}
}

//������ �������� üũ
UINT ThreadCheckNotResponseAck(LPVOID wParam)
{
	CMFCApplication1Dlg* main = (CMFCApplication1Dlg*)wParam;

	int nModule = 1;
	DWORD dVal = 0;
	while (true) {

		AxdiReadInportBit(nModule, 12, &dVal);
		
		if (dVal) {
			main->m_gLog->info(_T("NotResponseAck"));
		}

		Sleep(1);
	}
}



UINT ThreadWriteInkIO(LPVOID wParam)
{
	CMFCApplication1Dlg* main = (CMFCApplication1Dlg*)wParam;
	const int delayTime = 160;
	while (true) {
		if (main->m_bEventWriteInkIO) {
			Sleep(delayTime);
			main->m_mutex.lock();
			int cellNo = (int)main->m_queue.front();
			main->m_queue.pop();
			if (main->m_queue.size() > 15) {
				while (!main->m_queue.empty()) {
					main->m_queue.pop();
					main->m_gLog->info(_T("pop CellNo:{}"));
				}
			}
			main->m_mutex.unlock();
			main->WriteInkIO(cellNo, true, false);
			main->m_bEventWriteInkIO = false;
		}
		Sleep(1);
	}
}



class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

// �����Դϴ�.
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


// CMFCApplication1Dlg ��ȭ ����



CMFCApplication1Dlg::CMFCApplication1Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MFCAPPLICATION1_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMFCApplication1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()




BOOL CMFCApplication1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// �� ��ȭ ������ �������� �����մϴ�.  ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.

	m_gLog = std::make_shared<gLogger>("system_log", fmt::format(_T("{}{}"), "D:\\Log\\", "LOG.log"), false, 23, 59);
	m_gLog->set_pattern("[%H:%M:%S.%e] [%l] %v");
	m_gLog->setFileLevel(G_LOGGER_LEVEL_DEBUG);
	m_gLog->flush_on(spdlog::level::err);
	m_gLog->setConsoleLevel(G_LOGGER_LEVEL_DEBUG);
	m_gLog->info(_T("---------- Start I/O Test ----------"));
	
	bool bInitBoard = InitIOBoard();

	if (bInitBoard) {
		std::thread thread_readIO(ThreadReadIO, this);
		thread_readIO.detach();
		std::thread thread_checkNotAck(ThreadCheckNotResponseAck, this);
		thread_checkNotAck.detach();
		std::thread thread_writeInkIO(ThreadWriteInkIO, this);
		thread_writeInkIO.detach();
	}

	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}


bool CMFCApplication1Dlg::InitIOBoard()
{
	BOOL bRet = TRUE;

	DWORD Code = AxlOpenNoReset(7);
	if (Code == AXT_RT_SUCCESS)
	{
		TRACE("Library is initialized .\n");
		DWORD dwStatus;
		// Inspect if DIO module exsits
		Code = AxdInfoIsDIOModule(&dwStatus);
		if (dwStatus == STATUS_EXIST)
		{
			TRACE("DIO module exists.\n");
			long IModuleCounts;
			Code = AxdInfoGetModuleCount(&IModuleCounts);
			if (Code == AXT_RT_SUCCESS)
				TRACE("Number of DIO module: %d\n", IModuleCounts);
			else
				TRACE("AxdInfoGetModuleCount() : ERROR code Ox%x\n", Code);

			long IInputCounts;
			long IOutputCounts;
			long IBoardNo;
			long IModulePos;
			DWORD dwModuleID;

			CString strData;
			for (int ModuleNo = 0; ModuleNo < 2; ModuleNo++)
			{

				AxdInfoGetInputCount(ModuleNo, &IInputCounts);
				AxdInfoGetOutputCount(ModuleNo, &IOutputCounts);
				if (AxdInfoGetModule(ModuleNo, &IBoardNo, &IModulePos, &dwModuleID) == AXT_RT_SUCCESS)
				{
					m_gLog->info(_T("---------- ModuleNo:{} Success----------"), ModuleNo);
					switch (dwModuleID)
					{
					case AXT_SIO_DI32:
						strData.Format("[BD No:%d - MD No:%d] SIO_DI32", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDI32:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDI32", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_DO32P:
						strData.Format("[BD No:%d - MD No:%d] SIO_DO32P", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_DB32P:
						strData.Format("[BD No:%d - MD No:%d] SIO_DB32P", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_DO32T:
						strData.Format("[BD No:%d - MD No:%d] SIO_DO32T", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDO32:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDO32", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_DB32T:
						strData.Format("[BD No:%d - MD No:%d] SIO_DB32T", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDB128MLII:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDB128MLII", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RSIMPLEIOMLII:
						strData.Format("[BD No:%d - MD No:%d] SIO_RSIMPLEIOMLII", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDI16MLII:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDI16MLII", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDO16AMLII:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDO16AMLII", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDO16BMLII:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDO16BMLII", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDB96MLII:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDB96MLII", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDO32RTEX:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDO32RTEX", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDI32RTEX:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDI32RTEX", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDB32RTEX:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDB32RTEX", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_DI32_P:
						strData.Format("[BD No:%d - MD No:%d] SIO_DI32_P", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_DO32T_P:
						strData.Format("[BD No:%d - MD No:%d] SIO_DO32T_P", IBoardNo, ModuleNo);
						break;
					case AXT_SIO_RDB32T:
						strData.Format("[BD No:%d - MD No:%d] SIO_RDB32T", IBoardNo, ModuleNo);
						break;
					}

					//AxlInterruptEnable();
					//AxdiInterruptSetModuleEnable(ModuleNo, ENABLE);
					//AxdiInterruptEdgeSetWord(ModuleNo, 0, UP_EDGE, 0x0001);
					//AxdiInterruptSetModule(ModuleNo, this->GetSafeHwnd(), NULL, OnDIOInterruptCallback, NULL);
				}
				else {
					m_gLog->info(_T("---------- ModuleNo:{} Fail----------"), ModuleNo);
				}
			}
		}
		else {
			m_gLog->info(_T("---------- Board Not Exist----------"));
			bRet = FALSE;
		}
	}
	else {
		m_gLog->info(_T("---------- Board Init Fail ----------"));
		bRet = FALSE;
	}
	return bRet;
}

void CMFCApplication1Dlg::WriteInkIO(int cellNo, bool bNgNotch, bool bNgSurface)
{
	m_gLog->info(_T("WriteInkIO, CellNo:{}"), cellNo);

	for (int nModule = 0; nModule < 2; nModule++) {
		AxdoWriteOutportBit(nModule, 0, bNgNotch);		//bIsTabsNG: ġ��
		AxdoWriteOutportBit(nModule, 1, bNgSurface);	//bIsSurfNG: ǥ��

		//cellNo
		for (int i = 0; i <= 5; i++){
			int result = cellNo >> i & 1;
			if (result == 1){
				AxdoWriteOutportBit(nModule, i + 2, 1);
				AxdoWriteOutportBit(nModule, i + 2, 1);
			}
			else{
				AxdoWriteOutportBit(nModule, i + 2, 0);
				AxdoWriteOutportBit(nModule, i + 2, 0);
			}
		}
		//mark option
		AxdoWriteOutportBit(nModule, 9, true);
		AxdoWriteOutportBit(nModule, 10, false);

		
	}

	//pulse on
	for (int nModule = 0; nModule < 2; nModule++) {
		AxdoWriteOutportBit(nModule, 8, true);
		AxdoWriteOutportBit(nModule, 8, true);
	}
	Sleep(5);
	//pulse off
	for (int nModule = 0; nModule < 2; nModule++) {
		AxdoWriteOutportBit(nModule, 8, false);
		AxdoWriteOutportBit(nModule, 8, false);
	}


}

void CMFCApplication1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�.  ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CMFCApplication1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

