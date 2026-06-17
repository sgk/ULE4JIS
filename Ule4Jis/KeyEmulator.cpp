#include "StdAfx.h"
#include "KeyEmulator.h"
#include <imm.h>

#pragma comment(lib, "imm32.lib")

// IME control message constants
#define WM_IME_CONTROL 0x0283
#define IMC_GETOPENSTATUS 0x0005
#define IMC_SETOPENSTATUS 0x0006

static volatile LONG capsLockOffWorkerRunning = 0;

static void ClearCapsLockStateForCurrentThread() {
	BYTE keyState[256];
	GetKeyboardState(keyState);
	keyState[VK_CAPITAL] &= 0xfe;
	SetKeyboardState(keyState);
}

static void TurnCapsLockOffOnce() {
	if (::GetKeyState(VK_CAPITAL) & 0x0001) {
		::keybd_event(VK_CAPITAL, 0x45, 0, 0);
		::keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_KEYUP, 0);
	}
	ClearCapsLockStateForCurrentThread();
}

static DWORD WINAPI CapsLockOffWorkerProc(LPVOID) {
	static const DWORD delays[] = { 0, 10, 30, 60, 120, 250, 500, 1000 };
	for (int i = 0; i < sizeof(delays) / sizeof(delays[0]); ++i) {
		if (delays[i] != 0) {
			::Sleep(delays[i]);
		}
		TurnCapsLockOffOnce();
	}
	::InterlockedExchange(&capsLockOffWorkerRunning, 0);
	return 0;
}

KeyEmulator::KeyEmulator(EmulationStrategy *strategy) : capsLockMode(AltBackquote) {
	// initialize emulation map
	changeEmulationStrategy(strategy);
}

KeyEmulator::~KeyEmulator() {
}

void KeyEmulator::changeEmulationStrategy(EmulationStrategy *strategy) {
	strategy->getEmulationMap(&this->emulationMap);
}

void KeyEmulator::start() {
	ASSERT(this->hooker.get() == NULL);
	
	if (capsLockMode != Disabled) {
		scheduleCapsLockOff();
	}
	
	this->hooker.reset(new KeyHooker(this));
}

void KeyEmulator::end() {
	ASSERT(this->hooker.get() != NULL);
	this->hooker.reset();
}

bool KeyEmulator::onKeyHookEvent(const KeyHookEventArgs &args) {

	if (args.getExtraInfo() == (ULONG_PTR)this) {
		// ignore emulation input
		return false;
	}

	BYTE vkey = args.getVKey();

	// Toggle IME on/off when Caps Lock is pressed
	// Note: On Japanese keyboards, Caps Lock may be reported as 0xF0 instead of VK_CAPITAL (0x14)
	// On US keyboards, it should be VK_CAPITAL (0x14)
	if ((vkey == VK_CAPITAL || vkey == 0xF0) && capsLockMode != Disabled) {
		if (!args.isUp()) {
			if (capsLockMode == DirectIME) {
				toggleImeDirectly();
			} else {
				toggleImeOpenStatusForForegroundWindow();
			}
			scheduleCapsLockOff();
		} else {
			scheduleCapsLockOff();
		}
		return true;
	}

	// set condition object
	this->keyCondition.changeKeyState(args.getVKey(), args.isUp());

	// find emulation command object from map
	EmulationMapType::iterator it = this->emulationMap.find(this->keyCondition);
	if (it != this->emulationMap.end()) {
		// execute it
		if (args.isUp()) {
			it->second->executeUp(*this, this->keyCondition);
		} else {
			it->second->executeDown(*this, this->keyCondition);
		}

		return true; // cancel input
	}

	return false;
}

void KeyEmulator::emulateKey(BYTE vkey, bool up) const {
	DWORD flags = up ? KEYEVENTF_KEYUP : 0;
	if (isExtendedKey(vkey)) {
		flags |= KEYEVENTF_EXTENDEDKEY;
	}
	::keybd_event(vkey, 0, flags, (ULONG_PTR)this);
}

bool KeyEmulator::isExtendedKey(BYTE vkey) const {
	switch (vkey) {
		case VK_RCONTROL:
		case VK_RMENU:
		case VK_RSHIFT:
		case VK_INSERT:
		case VK_DELETE:
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_UP:
		case VK_DOWN:
		case VK_RIGHT:
		case VK_LEFT:
		case VK_NUMLOCK:
		case VK_CANCEL:
		case VK_PRINT:
		case VK_DIVIDE:
		case VK_SEPARATOR:
			return true;
		default:
			return false;
	}
}

void KeyEmulator::setCapsLockMode(CapsLockMode mode) {
	capsLockMode = mode;
	if (capsLockMode != Disabled) {
		scheduleCapsLockOff();
	}
}

void KeyEmulator::clearCapsLockState() const {
	BYTE keyState[256];
	GetKeyboardState(keyState);
	keyState[VK_CAPITAL] &= 0xfe;
	SetKeyboardState(keyState);
}

void KeyEmulator::turnCapsLockOff() const {
	if (::GetKeyState(VK_CAPITAL) & 0x0001) {
		::keybd_event(VK_CAPITAL, 0x45, 0, (ULONG_PTR)this);
		::keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_KEYUP, (ULONG_PTR)this);
	} else {
		clearCapsLockState();
	}
}

void KeyEmulator::scheduleCapsLockOff() const {
	if (::InterlockedCompareExchange(&capsLockOffWorkerRunning, 1, 0) != 0) {
		return;
	}

	HANDLE thread = ::CreateThread(NULL, 0, CapsLockOffWorkerProc, NULL, 0, NULL);
	if (thread != NULL) {
		::CloseHandle(thread);
	} else {
		::InterlockedExchange(&capsLockOffWorkerRunning, 0);
		turnCapsLockOff();
	}
}

bool KeyEmulator::toggleImeOpenStatusForForegroundWindow() {
	// AHK方式：半角/全角キー（sc029）を送信してIMEをトグル
	// これが日本語IMEの標準トグルキー

	INPUT input[2] = {};

	// 半角/全角キー（スキャンコード 0x29）
	WORD scanCode = 0x29;

	// Key down - スキャンコード指定が重要
	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wVk = 0;
	input[0].ki.wScan = scanCode;
	input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
	input[0].ki.dwExtraInfo = (ULONG_PTR)this;

	// Key up
	input[1].type = INPUT_KEYBOARD;
	input[1].ki.wVk = 0;
	input[1].ki.wScan = scanCode;
	input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	input[1].ki.dwExtraInfo = (ULONG_PTR)this;

	::SendInput(2, input, sizeof(INPUT));

	return true;
}

bool KeyEmulator::toggleImeDirectly() {
	HWND hwnd = ::GetForegroundWindow();
	if (hwnd == NULL) {
		return false;
	}

	// フォーカス中のコントロールのHWNDを取得（AHKスクリプトと同じロジック）
	GUITHREADINFO gti = {};
	gti.cbSize = sizeof(GUITHREADINFO);
	if (::GetGUIThreadInfo(0, &gti) && gti.hwndFocus != NULL) {
		hwnd = gti.hwndFocus;
	}

	// IMEのデフォルトウィンドウを取得してメッセージを送る方法（より確実）
	HWND imeWnd = ::ImmGetDefaultIMEWnd(hwnd);
	if (imeWnd != NULL) {
		// 現在のIME状態を取得
		LRESULT currentStatus = ::SendMessage(imeWnd, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0);
		// IME状態を反転
		::SendMessage(imeWnd, WM_IME_CONTROL, IMC_SETOPENSTATUS, currentStatus ? 0 : 1);
		return true;
	}

	// フォールバック: ImmGetContext/ImmSetOpenStatus を使う方法
	int currentStatus = getImeOpenStatus(hwnd);
	return setImeOpenStatus(hwnd, currentStatus == 0);
}

int KeyEmulator::getImeOpenStatus(HWND hwnd) {
	HIMC hImc = ::ImmGetContext(hwnd);
	if (hImc == NULL) {
		return 0;
	}

	BOOL isOpen = ::ImmGetOpenStatus(hImc);
	::ImmReleaseContext(hwnd, hImc);
	
	return isOpen ? 1 : 0;
}

bool KeyEmulator::setImeOpenStatus(HWND hwnd, bool open) {
	HIMC hImc = ::ImmGetContext(hwnd);
	if (hImc == NULL) {
		return false;
	}

	BOOL result = ::ImmSetOpenStatus(hImc, open ? TRUE : FALSE);
	::ImmReleaseContext(hwnd, hImc);
	
	return result != FALSE;
}
