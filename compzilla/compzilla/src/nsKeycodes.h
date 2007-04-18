
#include <gdk/gdkkeysyms.h>
#include <nsIDOMKeyEvent.h>


/*
 * From mozilla/widget/src/gtk/nsGtkEventHandler.cpp...
 */


//==============================================================
struct nsKeyConverter {
  int vkCode; // Platform independent key code
  int keysym; // GDK keysym key code
};


//
// Netscape keycodes are defined in widget/public/nsGUIEvent.h
// GTK keycodes are defined in <gdk/gdkkeysyms.h>
//
struct nsKeyConverter nsKeycodes[] = {
  { nsIDOMKeyEvent::DOM_VK_CANCEL,     GDK_Cancel },
  { nsIDOMKeyEvent::DOM_VK_BACK_SPACE, GDK_BackSpace },
  { nsIDOMKeyEvent::DOM_VK_TAB,        GDK_Tab },
  { nsIDOMKeyEvent::DOM_VK_TAB,        GDK_ISO_Left_Tab },
  { nsIDOMKeyEvent::DOM_VK_CLEAR,      GDK_Clear },
  { nsIDOMKeyEvent::DOM_VK_RETURN,     GDK_Return },
  { nsIDOMKeyEvent::DOM_VK_SHIFT,      GDK_Shift_L },
  { nsIDOMKeyEvent::DOM_VK_SHIFT,      GDK_Shift_R },
  { nsIDOMKeyEvent::DOM_VK_CONTROL,    GDK_Control_L },
  { nsIDOMKeyEvent::DOM_VK_CONTROL,    GDK_Control_R },
  { nsIDOMKeyEvent::DOM_VK_ALT,        GDK_Alt_L },
  { nsIDOMKeyEvent::DOM_VK_ALT,        GDK_Alt_R },
  { nsIDOMKeyEvent::DOM_VK_META,       GDK_Meta_L },
  { nsIDOMKeyEvent::DOM_VK_META,       GDK_Meta_R },
  { nsIDOMKeyEvent::DOM_VK_PAUSE,      GDK_Pause },
  { nsIDOMKeyEvent::DOM_VK_CAPS_LOCK,  GDK_Caps_Lock },
  { nsIDOMKeyEvent::DOM_VK_ESCAPE,     GDK_Escape },
  { nsIDOMKeyEvent::DOM_VK_SPACE,      GDK_space },
  { nsIDOMKeyEvent::DOM_VK_PAGE_UP,    GDK_Page_Up },
  { nsIDOMKeyEvent::DOM_VK_PAGE_DOWN,  GDK_Page_Down },
  { nsIDOMKeyEvent::DOM_VK_END,        GDK_End },
  { nsIDOMKeyEvent::DOM_VK_HOME,       GDK_Home },
  { nsIDOMKeyEvent::DOM_VK_LEFT,       GDK_Left },
  { nsIDOMKeyEvent::DOM_VK_UP,         GDK_Up },
  { nsIDOMKeyEvent::DOM_VK_RIGHT,      GDK_Right },
  { nsIDOMKeyEvent::DOM_VK_DOWN,       GDK_Down },
  { nsIDOMKeyEvent::DOM_VK_PRINTSCREEN, GDK_Print },
  { nsIDOMKeyEvent::DOM_VK_INSERT,     GDK_Insert },
  { nsIDOMKeyEvent::DOM_VK_DELETE,     GDK_Delete },

  // keypad keys
  { nsIDOMKeyEvent::DOM_VK_LEFT,       GDK_KP_Left },
  { nsIDOMKeyEvent::DOM_VK_RIGHT,      GDK_KP_Right },
  { nsIDOMKeyEvent::DOM_VK_UP,         GDK_KP_Up },
  { nsIDOMKeyEvent::DOM_VK_DOWN,       GDK_KP_Down },
  { nsIDOMKeyEvent::DOM_VK_PAGE_UP,    GDK_KP_Page_Up },
    // Not sure what these are
    //{ nsIDOMKeyEvent::DOM_VK_,       GDK_KP_Prior },
    //{ nsIDOMKeyEvent::DOM_VK_,        GDK_KP_Next },
    // GDK_KP_Begin is the 5 on the non-numlock keypad
    //{ nsIDOMKeyEvent::DOM_VK_,        GDK_KP_Begin },
  { nsIDOMKeyEvent::DOM_VK_PAGE_DOWN,  GDK_KP_Page_Down },
  { nsIDOMKeyEvent::DOM_VK_HOME,       GDK_KP_Home },
  { nsIDOMKeyEvent::DOM_VK_END,        GDK_KP_End },
  { nsIDOMKeyEvent::DOM_VK_INSERT,     GDK_KP_Insert },
  { nsIDOMKeyEvent::DOM_VK_DELETE,     GDK_KP_Delete },

  { nsIDOMKeyEvent::DOM_VK_MULTIPLY,   GDK_KP_Multiply },
  { nsIDOMKeyEvent::DOM_VK_ADD,        GDK_KP_Add },
  { nsIDOMKeyEvent::DOM_VK_SEPARATOR,  GDK_KP_Separator },
  { nsIDOMKeyEvent::DOM_VK_SUBTRACT,   GDK_KP_Subtract },
  { nsIDOMKeyEvent::DOM_VK_DECIMAL,    GDK_KP_Decimal },
  { nsIDOMKeyEvent::DOM_VK_DIVIDE,     GDK_KP_Divide },
  { nsIDOMKeyEvent::DOM_VK_RETURN,     GDK_KP_Enter },
  { nsIDOMKeyEvent::DOM_VK_NUM_LOCK,   GDK_Num_Lock },
  { nsIDOMKeyEvent::DOM_VK_SCROLL_LOCK,GDK_Scroll_Lock },

  { nsIDOMKeyEvent::DOM_VK_COMMA,      GDK_comma },
  { nsIDOMKeyEvent::DOM_VK_PERIOD,     GDK_period },
  { nsIDOMKeyEvent::DOM_VK_SLASH,      GDK_slash },
  { nsIDOMKeyEvent::DOM_VK_BACK_SLASH, GDK_backslash },
  { nsIDOMKeyEvent::DOM_VK_BACK_QUOTE, GDK_grave },
  { nsIDOMKeyEvent::DOM_VK_OPEN_BRACKET, GDK_bracketleft },
  { nsIDOMKeyEvent::DOM_VK_CLOSE_BRACKET, GDK_bracketright },
  { nsIDOMKeyEvent::DOM_VK_SEMICOLON, GDK_colon },
  { nsIDOMKeyEvent::DOM_VK_QUOTE, GDK_apostrophe },

  // context menu key, keysym 0xff67, typically keycode 117 on 105-key (Microsoft) 
  // x86 keyboards, located between right 'Windows' key and right Ctrl key
  { nsIDOMKeyEvent::DOM_VK_CONTEXT_MENU, GDK_Menu },

  // NS doesn't have dash or equals distinct from the numeric keypad ones,
  // so we'll use those for now.  See bug 17008:
  { nsIDOMKeyEvent::DOM_VK_SUBTRACT, GDK_minus },
  { nsIDOMKeyEvent::DOM_VK_EQUALS, GDK_equal },

  // Some shifted keys, see bug 15463 as well as 17008.
  // These should be subject to different keyboard mappings.
  { nsIDOMKeyEvent::DOM_VK_QUOTE, GDK_quotedbl },
  { nsIDOMKeyEvent::DOM_VK_OPEN_BRACKET, GDK_braceleft },
  { nsIDOMKeyEvent::DOM_VK_CLOSE_BRACKET, GDK_braceright },
  { nsIDOMKeyEvent::DOM_VK_BACK_SLASH, GDK_bar },
  { nsIDOMKeyEvent::DOM_VK_SEMICOLON, GDK_semicolon },
  { nsIDOMKeyEvent::DOM_VK_BACK_QUOTE, GDK_asciitilde },
  { nsIDOMKeyEvent::DOM_VK_COMMA, GDK_less },
  { nsIDOMKeyEvent::DOM_VK_PERIOD, GDK_greater },
  { nsIDOMKeyEvent::DOM_VK_SLASH,      GDK_question },
  { nsIDOMKeyEvent::DOM_VK_1, GDK_exclam },
  { nsIDOMKeyEvent::DOM_VK_2, GDK_at },
  { nsIDOMKeyEvent::DOM_VK_3, GDK_numbersign },
  { nsIDOMKeyEvent::DOM_VK_4, GDK_dollar },
  { nsIDOMKeyEvent::DOM_VK_5, GDK_percent },
  { nsIDOMKeyEvent::DOM_VK_6, GDK_asciicircum },
  { nsIDOMKeyEvent::DOM_VK_7, GDK_ampersand },
  { nsIDOMKeyEvent::DOM_VK_8, GDK_asterisk },
  { nsIDOMKeyEvent::DOM_VK_9, GDK_parenleft },
  { nsIDOMKeyEvent::DOM_VK_0, GDK_parenright },
  { nsIDOMKeyEvent::DOM_VK_SUBTRACT, GDK_underscore },
  { nsIDOMKeyEvent::DOM_VK_EQUALS, GDK_plus }
};


#define IS_XSUN_XSERVER(dpy) \
    (strstr(XServerVendor(dpy), "Sun Microsystems") != NULL)


// map Sun Keyboard special keysyms on to nsIDOMKeyEvent::DOM_VK keys
struct nsKeyConverter nsSunKeycodes[] = {
  {nsIDOMKeyEvent::DOM_VK_ESCAPE, GDK_F11 }, //bug 57262, Sun Stop key generates F11 keysym
  {nsIDOMKeyEvent::DOM_VK_F11, 0x1005ff10 }, //Sun F11 key generates SunF36(0x1005ff10) keysym
  {nsIDOMKeyEvent::DOM_VK_F12, 0x1005ff11 }, //Sun F12 key generates SunF37(0x1005ff11) keysym
  {nsIDOMKeyEvent::DOM_VK_PAGE_UP,    GDK_F29 }, //KP_Prior
  {nsIDOMKeyEvent::DOM_VK_PAGE_DOWN,  GDK_F35 }, //KP_Next
  {nsIDOMKeyEvent::DOM_VK_HOME,       GDK_F27 }, //KP_Home
  {nsIDOMKeyEvent::DOM_VK_END,        GDK_F33 }, //KP_End
};


