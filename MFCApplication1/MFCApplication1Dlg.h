
// MFCApplication1Dlg.h : ��� ����
//

#pragma once

#include "AXD.h"
#include "AXL.h"
#include <gLogger.h>
#include <memory>

#pragma comment(lib, "AXTLIB.lib")
#pragma comment(lib, "AXL.lib")


// CMFCApplication1Dlg ��ȭ ����
class CMFCApplication1Dlg : public CDialogEx
{
// �����Դϴ�.
public:
	CMFCApplication1Dlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

	bool    m_bOldTrgVal = false;
	int		m_nCellNo = 0;
	bool	m_bEventWriteInkIO = false;
	void	WriteInkIO(int cellNo, bool bNgNotch, bool bNgSurface);
	std::shared_ptr<gLogger> m_gLog;

// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCAPPLICATION1_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;


	bool	InitIOBoard();

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
