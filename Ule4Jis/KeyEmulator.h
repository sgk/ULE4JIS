#pragma once

#include "EmulationStrategy.h"
#include "KeyHooker.h"
#include "KeyHookEventListener.h"
#include "KeyCondition.h"

class KeyEmulator : public KeyHookEventListener {
public:
	enum CapsLockMode { AltBackquote, DirectIME, Disabled };

private:
	EmulationMapType emulationMap;
	std::auto_ptr<KeyHooker> hooker;
	KeyCondition keyCondition;
	CapsLockMode capsLockMode;

private:
	bool isExtendedKey(BYTE vkey) const;
	bool toggleImeOpenStatusForForegroundWindow();
	bool toggleImeDirectly();
	int getImeOpenStatus(HWND hwnd);
	bool setImeOpenStatus(HWND hwnd, bool open);

public:
	KeyEmulator(EmulationStrategy *strategy);
	~KeyEmulator();

	void changeEmulationStrategy(EmulationStrategy *strategy);
	void start();
	void end();
	virtual bool onKeyHookEvent(const KeyHookEventArgs &args);
	bool isStarted() const { return (this->hooker.get() != NULL); }

	void emulateKey(BYTE vkey, bool up = false) const;

	void setCapsLockMode(CapsLockMode mode) { capsLockMode = mode; }
	CapsLockMode getCapsLockMode() const { return capsLockMode; }
};
