// Ule4JisDlg.h : ヘッダー ファイル
//

#pragma once

#include "KeyEmulator.h"

class CAboutDlg;

// Ule4JisDlg ダイアログ
class Ule4JisDlg : public CDialog
{
// コンストラクション
public:
	Ule4JisDlg(CWnd* pParent = NULL);	// 標準コンストラクタ

// ダイアログ データ
	enum { IDD = IDD_ULE4JP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート

private:
	// added
	enum Strategy { USonJIS, JISonUS };
	enum CapsLockMode { AltBackquote, DirectIME, Disabled };
	NOTIFYICONDATA notifyIconData;
	Strategy currentStrategy;
	CapsLockMode capsLockMode;
	CAboutDlg* pSplashDlg;

	void showTaskTrayPopupMenu();
	void changeTaskTrayIconToUS();
	void changeTaskTrayIconToJIS();
	void saveSettings();
	void loadSettings();

// 実装
protected:
	HICON m_hIcon;
	std::auto_ptr<KeyEmulator> keyEmulator;

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL DestroyWindow();
protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedHide();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
};
