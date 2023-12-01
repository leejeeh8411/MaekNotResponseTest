
// MFCApplication1Dlg.h : 헤더 파일
//

#pragma once

#include "AXD.h"
#include "AXL.h"
#include <gLogger.h>
#include <memory>

#pragma comment(lib, "AXTLIB.lib")
#pragma comment(lib, "AXL.lib")


// CMFCApplication1Dlg 대화 상자
class CMFCApplication1Dlg : public CDialogEx
{
// 생성입니다.
public:
	CMFCApplication1Dlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

	bool    m_bOldTrgVal = false;
	int		m_nCellNo = 0;
	bool	m_bEventWriteInkIO = false;
	void	WriteInkIO(int cellNo, bool bNgNotch, bool bNgSurface);
	std::shared_ptr<gLogger> m_gLog;

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCAPPLICATION1_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;


	bool	InitIOBoard();

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
