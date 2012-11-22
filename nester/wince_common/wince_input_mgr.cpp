#include "nesterce.h"
#include "wince_input_mgr.h"
#include "debug.h"

extern void RegisterKeyListener(KeyListener *listener);
extern void UnregisterKeyListener(KeyListener *listener);


wince_input_mgr::wince_input_mgr()
{
	thaw();
}

wince_input_mgr::~wince_input_mgr()
{
	freeze();
}

void wince_input_mgr::Poll()
{
}

void wince_input_mgr::freeze()
{
#ifdef _USES_GAPI_INPUT
	GXCloseInput();
#endif
}

void wince_input_mgr::thaw()
{
#ifdef _USES_GAPI_INPUT
	GXOpenInput();
#endif
}

wince_INPButton::wince_INPButton(wince_input_mgr* inp_mgr, OSD_ButtonSettings* settings)								 
{
	inputMgr = inp_mgr;
	m_key = settings->key;
	turbo = settings->turbo;
	turbosw = 0;
}

wince_INPButton::~wince_INPButton()
{
}

int wince_INPButton::Pressed()
{
	if (turbo) {
		if (GetAsyncKeyState(m_key) & 0x8000) {
			turbosw = !turbosw;
			return turbosw ? 0x80 : 0;
		}
		return 0;
	}
	return (GetAsyncKeyState(m_key) & 0x8000) ? 0x80 : 0;
}

wince_PassiveButton::wince_PassiveButton(wince_input_mgr* inp_mgr, OSD_ButtonSettings* settings) :
    wince_INPButton(inp_mgr, settings), keyDown(0)
{
    RegisterKeyListener(this);
}

wince_PassiveButton::~wince_PassiveButton()
{
    UnregisterKeyListener(this);
}

int wince_PassiveButton::keyEvent(int keyDown)
{
    this->keyDown = keyDown;
    return 0;
}

int wince_PassiveButton::Pressed()
{
    if (turbo) {
        if (keyDown) {
            turbosw = !turbosw;
            return turbosw ? 0x80 : 0;
        }
        return 0;
    }
    return keyDown ? 0x80 : 0;
}
