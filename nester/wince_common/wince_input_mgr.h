#ifndef __WINCE_PPC_INPUT_MGR_H__
#define __WINCE_PPC_INPUT_MGR_H__

#include <windows.h>
#include "input_mgr.h"
#include "INPButton.h"
#include "OSD_ButtonSettings.h"

class wince_input_mgr : public input_mgr
{
public:
	wince_input_mgr();
	~wince_input_mgr();
	void Poll();

	void freeze();
	void thaw();
};

class wince_INPButton_None : public INPButton
{
public:
	int Pressed() {return 0;}
};

class wince_INPButton : public INPButton
{
public:
	wince_INPButton(wince_input_mgr* inp_mgr, OSD_ButtonSettings* settings);
	~wince_INPButton();
	int Pressed();

protected:
	uint8 m_key;
	wince_input_mgr* inputMgr;

	int turbo;
	int turbosw;

private:
};

class KeyListener
{
public:
    virtual int keyEvent(int keyDown) = 0;
    virtual int getKeyCode() = 0;
};

class wince_PassiveButton : public wince_INPButton, KeyListener
{
public:
    wince_PassiveButton(wince_input_mgr* inp_mgr, OSD_ButtonSettings* settings);
    ~wince_PassiveButton();
    int keyEvent(int keyDown);
    int getKeyCode()
    {
        return m_key;
    }
    int Pressed();

protected:
    int keyDown;
};

#endif //__WINCE_PPC_INPUT_MGR_H__
