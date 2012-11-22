#include "wince_NES_pad.h"
#include "debug.h"
#include "settings.h"

INPButton* GetINPButton(OSD_ButtonSettings* settings, wince_input_mgr* inp_mgr)
{
	switch(settings->type)
	{
	case OSD_ButtonSettings::KEYBOARD_KEY:
        if (NESTER_settings.nes.input.wince_altKeyScan)
		    return new wince_PassiveButton(inp_mgr, settings);
        else
            return new wince_INPButton(inp_mgr, settings);
        break;
	}
	return new wince_INPButton_None();
}

wince_NES_pad::wince_NES_pad(NES_controller_input_settings* settings, NES_pad* pad, wince_input_mgr* inp_mgr)
{
	m_pad = pad;
	m_ButtonUp = m_ButtonDown = m_ButtonLeft = m_ButtonRight =
	m_ButtonSelect = m_ButtonStart = m_ButtonB = m_ButtonA = NULL;
	CreateButtons(settings, inp_mgr);
}

wince_NES_pad::~wince_NES_pad()
{
	DeleteButtons();
}

void wince_NES_pad::Poll()
{
	m_pad->set_button_state(NES_UP,     m_ButtonUp->Pressed());
	m_pad->set_button_state(NES_DOWN,   m_ButtonDown->Pressed());
	m_pad->set_button_state(NES_LEFT,   m_ButtonLeft->Pressed());
	m_pad->set_button_state(NES_RIGHT,  m_ButtonRight->Pressed());
	m_pad->set_button_state(NES_SELECT, m_ButtonSelect->Pressed());
	m_pad->set_button_state(NES_START,  m_ButtonStart->Pressed());
	m_pad->set_button_state(NES_B,      m_ButtonB->Pressed());
	m_pad->set_button_state(NES_A,      m_ButtonA->Pressed());
}

INPButton* wince_NES_pad::CreateButton(OSD_ButtonSettings* settings, wince_input_mgr* inp_mgr)
{
	INPButton* button;

	button = GetINPButton(settings, inp_mgr);
	if(!button) THROW_EXCEPTION;

	return button;
}


void wince_NES_pad::CreateButtons(NES_controller_input_settings* settings,
                                  wince_input_mgr* inp_mgr)
{
	DeleteButtons();

	PN_TRY {
		m_ButtonUp     = CreateButton(&settings->btnUp, inp_mgr);
		m_ButtonDown   = CreateButton(&settings->btnDown, inp_mgr);
		m_ButtonLeft   = CreateButton(&settings->btnLeft, inp_mgr);
		m_ButtonRight  = CreateButton(&settings->btnRight, inp_mgr);
		m_ButtonSelect = CreateButton(&settings->btnSelect, inp_mgr);
		m_ButtonStart  = CreateButton(&settings->btnStart, inp_mgr);
		m_ButtonB      = CreateButton(&settings->btnB, inp_mgr);
		m_ButtonA      = CreateButton(&settings->btnA, inp_mgr);
	}
	PN_CATCH { 
		errorlog("pad creation error\n");
		DeleteButtons();
		THROW_EXCEPTION;
	}
}


#define DELETEBUTTON(ptr) \
  if(ptr) delete ptr; \
  ptr = NULL;

void wince_NES_pad::DeleteButtons()
{
  DELETEBUTTON(m_ButtonUp);
  DELETEBUTTON(m_ButtonDown);
  DELETEBUTTON(m_ButtonLeft);
  DELETEBUTTON(m_ButtonRight);
  DELETEBUTTON(m_ButtonSelect);
  DELETEBUTTON(m_ButtonStart);
  DELETEBUTTON(m_ButtonB);
  DELETEBUTTON(m_ButtonA);
}


