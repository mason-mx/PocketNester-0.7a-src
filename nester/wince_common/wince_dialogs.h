#ifndef _WINCE_DIALOGS_H__
#define _WINCE_DIALOGS_H__

BOOL CALLBACK AboutNester_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PreferencesOptions_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GraphicsOptions_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SoundOptions_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ControllersOptions_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DirectoryOptions_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif