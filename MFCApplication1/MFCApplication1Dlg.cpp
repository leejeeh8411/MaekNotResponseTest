
// MFCApplication1Dlg.cpp : 구현 파일
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

//미응답 들어오는지 체크
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

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
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


// CMFCApplication1Dlg 대화 상자



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

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
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

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

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

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
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
		AxdoWriteOutportBit(nModule, 0, bNgNotch);		//bIsTabsNG: 치수
		AxdoWriteOutportBit(nModule, 1, bNgSurface);	//bIsSurfNG: 표면

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

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CMFCApplication1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

