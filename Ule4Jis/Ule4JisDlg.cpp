// Ule4JisDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "Ule4Jis.h"
#include "Ule4JisDlg.h"
#include "KeyEmulator.h"
#include "USonJISStrategy.h"
#include "NopStrategy.h"
#include "Constants.h"
#include "afxwin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	void SetModeless(bool modeless) { m_bModeless = modeless; }

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
	CStatic urlLabel;
	CStatic githubUrlLabel;
	HCURSOR handCursor;
	bool m_bModeless;
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD), m_bModeless(false)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ABOUT_URL, urlLabel);
	DDX_Control(pDX, IDC_GITHUB_URL, githubUrlLabel);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_CTLCOLOR()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()


// Ule4JisDlg ダイアログ




Ule4JisDlg::Ule4JisDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Ule4JisDlg::IDD, pParent), capsLockMode(AltBackquote), currentStrategy(USonJIS), pSplashDlg(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ule4JisDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Ule4JisDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_HIDE, &Ule4JisDlg::OnBnClickedHide)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
END_MESSAGE_MAP()


// Ule4JisDlg メッセージ ハンドラ

BOOL Ule4JisDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。

	// add icon into task tray
	NOTIFYICONDATA &nid = this->notifyIconData;
	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = this->m_hWnd;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = MSG_TASKTRAY_CALLBACK;
	nid.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	CString title;
	GetWindowText(title);
	_tcscpy_s(nid.szTip, sizeof(nid.szTip) / sizeof(TCHAR), title);

	if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
		MessageBox(_T("failed to initialize tasktray icon."), NULL, MB_OK | MB_ICONEXCLAMATION);
		::PostQuitMessage(-1);
		return FALSE;
	}

	// initialize emulator
	USonJISStrategy strategy;
	this->keyEmulator.reset(new KeyEmulator(&strategy));

	// load settings
	loadSettings();

	this->keyEmulator->start();

	// スプレッシュスクリーンを非同期で表示（タイトルバーなし）
	pSplashDlg = new CAboutDlg();
	pSplashDlg->SetModeless(true);  // モードレスフラグを設定
	if (pSplashDlg->Create(IDD_ABOUTBOX, NULL)) {
		// タイトルバーと縁を削除
		pSplashDlg->ModifyStyle(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU, 0);
		pSplashDlg->ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0);
		pSplashDlg->SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		pSplashDlg->CenterWindow(CWnd::GetDesktopWindow());
		pSplashDlg->ShowWindow(SW_SHOW);
		pSplashDlg->UpdateWindow();
	}

	// 1秒後にスプラッシュを閉じるタイマーを設定
	SetTimer(1, 1000, NULL);

	// start hidden: run as tray resident app from launch
	ShowWindow(SW_HIDE);


	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void Ule4JisDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void Ule4JisDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR Ule4JisDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL Ule4JisDlg::DestroyWindow()
{
	// save settings
	saveSettings();

	// delete splash dialog if still exists
	if (pSplashDlg != NULL) {
		pSplashDlg->DestroyWindow();
		delete pSplashDlg;
		pSplashDlg = NULL;
	}

	// delete icon from tasktray
	::Shell_NotifyIcon(NIM_DELETE, &this->notifyIconData);

	return CDialog::DestroyWindow();
}

LRESULT Ule4JisDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case MSG_TASKTRAY_CALLBACK:
		// dispatch tasktray callback message
		switch (lParam) {
		case WM_RBUTTONUP:
		case WM_LBUTTONUP:
			// show popup menu on left or right click
			showTaskTrayPopupMenu();
			break;
		default:
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_TASKTRAY_ABOUT:
			{
				CAboutDlg dlgAbout;
				dlgAbout.DoModal();
			}
			break;
		case ID_TASKTRAY_ENABLE:
			if (this->keyEmulator->isStarted()) {
				this->keyEmulator->end();
				changeTaskTrayIconToJIS();
			} else {
				this->keyEmulator->start();
				changeTaskTrayIconToUS();
			}
			break;
		case ID_TASKTRAY_USONJIS:
			{
				bool wasStarted = this->keyEmulator->isStarted();
				if (wasStarted) {
					this->keyEmulator->end();
				}

				if (this->currentStrategy == USonJIS) {
					// Switch to NopStrategy (disable US on JIS)
					this->currentStrategy = JISonUS;
					NopStrategy nopStrategy;
					this->keyEmulator->changeEmulationStrategy(&nopStrategy);
				} else {
					// Switch back to USonJIS
					this->currentStrategy = USonJIS;
					USonJISStrategy usStrategy;
					this->keyEmulator->changeEmulationStrategy(&usStrategy);
				}

				if (wasStarted) {
					this->keyEmulator->start();
				}
			}
			break;
		case ID_TASKTRAY_CAPSLOCK_ALT_BACKQUOTE:
			capsLockMode = AltBackquote;
			this->keyEmulator->setCapsLockMode(KeyEmulator::AltBackquote);
			break;
		case ID_TASKTRAY_CAPSLOCK_DIRECT_IME:
			capsLockMode = DirectIME;
			this->keyEmulator->setCapsLockMode(KeyEmulator::DirectIME);
			break;
		case ID_TASKTRAY_CAPSLOCK_DISABLED:
			capsLockMode = Disabled;
			this->keyEmulator->setCapsLockMode(KeyEmulator::Disabled);
			break;
		case ID_TASKTRAY_EXIT:
			PostMessage(WM_CLOSE, 0, 0);
			break;
		default:
			break;
		}
	}

	return CDialog::WindowProc(message, wParam, lParam);
}

void Ule4JisDlg::changeTaskTrayIconToUS() {
	this->notifyIconData.hIcon = ::AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	::Shell_NotifyIcon(NIM_MODIFY, &this->notifyIconData);
}

void Ule4JisDlg::changeTaskTrayIconToJIS() {
	this->notifyIconData.hIcon = ::AfxGetApp()->LoadIcon(IDR_ICON_JIS);
	::Shell_NotifyIcon(NIM_MODIFY, &this->notifyIconData);
}

void Ule4JisDlg::showTaskTrayPopupMenu() {
	CPoint point;
	GetCursorPos(&point);

	CMenu menu;
	menu.CreatePopupMenu();

	// このアプリについて...
	menu.AppendMenu(MF_STRING, ID_TASKTRAY_ABOUT, _T("このアプリについて..."));
	menu.AppendMenu(MF_SEPARATOR);

	// 有効
	UINT enableFlags = MF_STRING;
	if (this->keyEmulator->isStarted()) {
		enableFlags |= MF_CHECKED;
	}
	menu.AppendMenu(enableFlags, ID_TASKTRAY_ENABLE, _T("有効"));

	// 日本語キーボードでUS配列
	UINT usOnJisFlags = MF_STRING;
	if (this->currentStrategy == USonJIS) {
		usOnJisFlags |= MF_CHECKED;
	}
	menu.AppendMenu(usOnJisFlags, ID_TASKTRAY_USONJIS, _T("日本語キーボードでUS配列"));

	menu.AppendMenu(MF_SEPARATOR);

	// Caps Lockの動作 (見出しのみ、選択不可)
	menu.AppendMenu(MF_OWNERDRAW | MF_DISABLED, ID_TASKTRAY_CAPSLOCK_HEADER, _T("Caps Lockの動作"));

	// Alt + ` (従来通りの動作)
	UINT altBackquoteFlags = MF_STRING;
	if (capsLockMode == AltBackquote) {
		altBackquoteFlags |= MF_CHECKED;
	}
	menu.AppendMenu(altBackquoteFlags, ID_TASKTRAY_CAPSLOCK_ALT_BACKQUOTE, _T("Alt + バッククオート"));

	// IMEを直接オンオフ (新しい動作)
	UINT directImeFlags = MF_STRING;
	if (capsLockMode == DirectIME) {
		directImeFlags |= MF_CHECKED;
	}
	menu.AppendMenu(directImeFlags, ID_TASKTRAY_CAPSLOCK_DIRECT_IME, _T("IMEを直接オンオフ"));

	// 無効
	UINT disabledFlags = MF_STRING;
	if (capsLockMode == Disabled) {
		disabledFlags |= MF_CHECKED;
	}
	menu.AppendMenu(disabledFlags, ID_TASKTRAY_CAPSLOCK_DISABLED, _T("無効"));

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, ID_TASKTRAY_EXIT, _T("終了"));

	// メニューを表示する前にフォアグラウンドウィンドウに設定
	// これにより、メニューがフォーカスを失ったときに確実に閉じる
	SetForegroundWindow();
	menu.TrackPopupMenu(TPM_BOTTOMALIGN | TPM_RIGHTALIGN, point.x, point.y, this);
	// メニューが閉じられることを保証するため、ダミーメッセージを送信
	PostMessage(WM_NULL, 0, 0);
}

void Ule4JisDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// Always keep dialog hidden - this app runs in system tray only
	if (nType == SIZE_MINIMIZED) {
		ShowWindow(SW_HIDE);
	}
}

void Ule4JisDlg::OnBnClickedHide()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	ShowWindow(SW_MINIMIZE);
}

void Ule4JisDlg::saveSettings()
{
	// Save settings to registry
	HKEY hKey;
	LONG result = RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ULE4JIS"), 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);

	if (result == ERROR_SUCCESS) {
		DWORD enabled = this->keyEmulator->isStarted() ? 1 : 0;
		RegSetValueEx(hKey, _T("Enabled"), 0, REG_DWORD, (BYTE*)&enabled, sizeof(DWORD));

		DWORD capsLock = static_cast<DWORD>(capsLockMode);
		RegSetValueEx(hKey, _T("CapsLockMode"), 0, REG_DWORD, (BYTE*)&capsLock, sizeof(DWORD));

		DWORD strategy = (this->currentStrategy == USonJIS) ? 1 : 0;
		RegSetValueEx(hKey, _T("USonJIS"), 0, REG_DWORD, (BYTE*)&strategy, sizeof(DWORD));

		RegCloseKey(hKey);
	}
}

void Ule4JisDlg::loadSettings()
{
	// Load settings from registry
	HKEY hKey;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ULE4JIS"), 0, KEY_READ, &hKey);

	if (result == ERROR_SUCCESS) {
		DWORD enabled = 1;
		DWORD size = sizeof(DWORD);
		RegQueryValueEx(hKey, _T("Enabled"), NULL, NULL, (BYTE*)&enabled, &size);

		DWORD capsLock = 0;
		size = sizeof(DWORD);
		RegQueryValueEx(hKey, _T("CapsLockMode"), NULL, NULL, (BYTE*)&capsLock, &size);

		DWORD strategy = 1;
		size = sizeof(DWORD);
		RegQueryValueEx(hKey, _T("USonJIS"), NULL, NULL, (BYTE*)&strategy, &size);

		capsLockMode = static_cast<CapsLockMode>(capsLock);
		this->keyEmulator->setCapsLockMode(static_cast<KeyEmulator::CapsLockMode>(capsLock));

		// Apply strategy
		if (strategy != 0) {
			this->currentStrategy = USonJIS;
			USonJISStrategy usStrategy;
			this->keyEmulator->changeEmulationStrategy(&usStrategy);
		} else {
			this->currentStrategy = JISonUS;
			NopStrategy nopStrategy;
			this->keyEmulator->changeEmulationStrategy(&nopStrategy);
		}

		if (!enabled) {
			this->keyEmulator->end();
			changeTaskTrayIconToJIS();
		}

		RegCloseKey(hKey);
	}
}

HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  ここで DC の属性を変更してください。

	// TODO:  既定値を使用したくない場合は別のブラシを返します。

	// set color 'blue' to draw URL text.
	if (pWnd == &this->urlLabel || pWnd == &this->githubUrlLabel) {
		pDC->SetTextColor(RGB(0, 0, 0xFF));
	}

	return hbr;
}

BOOL CAboutDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_ABOUT_URL) {
		if (HIWORD(wParam) == STN_CLICKED) {
			HINSTANCE result = ::ShellExecute(NULL, _T("open"), DEZZ_NETWORKS_URL, NULL, NULL, SW_SHOWNORMAL);
			if ((LONG)result <= 32) {
				// error. but since this is not critical problem, i ignore this :P
			}
		}
	}
	else if (LOWORD(wParam) == IDC_GITHUB_URL) {
		if (HIWORD(wParam) == STN_CLICKED) {
			HINSTANCE result = ::ShellExecute(NULL, _T("open"), _T("https://github.com/sgk/ULE4JIS"), NULL, NULL, SW_SHOWNORMAL);
			if ((LONG)result <= 32) {
				// error. but since this is not critical problem, i ignore this :P
			}
		}
	}

	return CDialog::OnCommand(wParam, lParam);
}

BOOL CAboutDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// set hand cursor if a pointer is over url-label
	if (pWnd == &this->urlLabel || pWnd == &this->githubUrlLabel) {
		SetCursor(this->handCursor);
		return TRUE;
	}

	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// タイトルバーと縁を削除してシンプルな四角いウィンドウに
	ModifyStyle(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU, 0);
	ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0);

	// ウィンドウを再描画してスタイル変更を反映
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	// get hand cursor handle
	// Suppress C4302: MAKEINTRESOURCE is standard Win32 pattern for cursor/icon loading
	// Warning is false positive for Win32 (x86) target - no actual truncation occurs
#pragma warning(push)
#pragma warning(disable: 4302)
	this->handCursor = ::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND));
#pragma warning(pop)

	return TRUE;
}

void CAboutDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// モーダル時のみ左クリックでウィンドウを閉じる
	if (!m_bModeless) {
		EndDialog(IDOK);
	}
}

void CAboutDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// モーダル時のみ右クリックでウィンドウを閉じる
	if (!m_bModeless) {
		EndDialog(IDOK);
	}
}

void CAboutDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

	// モーダル時のみバックグラウンドになったら閉じる
	if (!m_bModeless && nState == WA_INACTIVE) {
		EndDialog(IDOK);
	}
}

BOOL CAboutDlg::PreTranslateMessage(MSG* pMsg)
{
	// モーダル時のみキーボード入力でウィンドウを閉じる
	if (!m_bModeless && (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)) {
		EndDialog(IDOK);
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void Ule4JisDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		// スプラッシュスクリーンを閉じる
		KillTimer(1);
		if (pSplashDlg != NULL) {
			pSplashDlg->DestroyWindow();
			delete pSplashDlg;
			pSplashDlg = NULL;
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void Ule4JisDlg::OnClose()
{
	// DestroyWindow will call saveSettings(), then quit the app
	DestroyWindow();
	PostQuitMessage(0);
}

void Ule4JisDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct->CtlType == ODT_MENU && lpDrawItemStruct->itemID == ID_TASKTRAY_CAPSLOCK_HEADER) {
		CDC dc;
		dc.Attach(lpDrawItemStruct->hDC);
		CRect rect(lpDrawItemStruct->rcItem);
		::FillRect(dc.GetSafeHdc(), rect, ::GetSysColorBrush(COLOR_MENU));

		int oldBkMode = dc.SetBkMode(TRANSPARENT);
		COLORREF oldTextColor = dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
		CFont* pOldFont = dc.SelectObject(GetFont());

		CString text(reinterpret_cast<LPCTSTR>(lpDrawItemStruct->itemData));
		if (text.IsEmpty()) {
			text = _T("Caps Lockの動作");
		}
		rect.left += 8;
		dc.DrawText(text, rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

		dc.SelectObject(pOldFont);
		dc.SetTextColor(oldTextColor);
		dc.SetBkMode(oldBkMode);
		dc.Detach();
		return;
	}

	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void Ule4JisDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (lpMeasureItemStruct->CtlType == ODT_MENU && lpMeasureItemStruct->itemID == ID_TASKTRAY_CAPSLOCK_HEADER) {
		CClientDC dc(this);
		CFont* pOldFont = dc.SelectObject(GetFont());
		CSize size = dc.GetTextExtent(_T("Caps Lockの動作"));
		lpMeasureItemStruct->itemWidth = size.cx;
		lpMeasureItemStruct->itemHeight = size.cy + 4;
		dc.SelectObject(pOldFont);
		return;
	}

	CDialog::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}
