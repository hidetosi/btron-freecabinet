#define KEY_UP	0x0100
#define KEY_DOWN	0x0101
#define KEY_RIGHT	0x0102
#define KEY_LEFT	0x0103

extern void DeleteSelectedData();
extern void PushWorkArea();
extern BOOL KeyDown();
extern void SwitchFullWindow();
extern void NullEvent();
extern void BackgroundSelectedVobj();
extern BOOL EndProg();
extern void ErrPanel();


extern BOOL EventExec();


#define RIGHTBAR 0
#define BOTTOMBAR 1

extern BOOL SetBarValue();
extern void ActScrollBar();
extern void ScrollByBarDifference();
extern void BarMove();


extern void DrawSelectFrame();
extern BOOL MakeSelectFrame();
extern void FreeSelectFrame();
extern void DrawOneVobj();
extern void DrawOneRect();
extern void DrawView();
extern void DrawSelectedData();


extern BOOL MenuInit();
extern BOOL OpenMenu();
extern BOOL MenuKey();
extern void SwitchFullWindowIndicator();
extern BOOL GetFullWindowIndicator();
