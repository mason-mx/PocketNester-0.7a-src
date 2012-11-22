#ifndef WIN32_NES_PAD_H_
#define WIN32_NES_PAD_H_

#include "NES_pad.h"
#include "NES_settings.h"
#include "wince_input_mgr.h"
#include "INPButton.h"

// ack. ptooie.

class wince_NES_pad
{
public:
  wince_NES_pad(NES_controller_input_settings* settings, NES_pad* pad, wince_input_mgr* inp_mgr);
  virtual ~wince_NES_pad();

  void Poll();

protected:
  NES_pad* m_pad;

  INPButton* m_ButtonUp;
  INPButton* m_ButtonDown;
  INPButton* m_ButtonLeft;
  INPButton* m_ButtonRight;
  INPButton* m_ButtonSelect;
  INPButton* m_ButtonStart;
  INPButton* m_ButtonB;
  INPButton* m_ButtonA;

  INPButton* CreateButton(OSD_ButtonSettings* settings, wince_input_mgr* inp_mgr);

  void CreateButtons(NES_controller_input_settings* settings, wince_input_mgr* inp_mgr);
  void DeleteButtons();

private:
};

#endif
