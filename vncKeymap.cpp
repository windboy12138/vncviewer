#include "vnckeymap.h"

#include <Windows.h>
#include <ime.h>
#include <map>
#include <stdlib.h>
#include <vector>

#define XK_MISCELLANY
#define XK_LATIN1
#define XK_CURRENCY
#define XK_GREEK
#define XK_TECHNICAL
#define XK_XKB_KEYS
#define XK_KATAKANA
#include "keysymdef.h"



// Mapping of X keysyms to windows VK codes.  Ordering here is the same as
// keysymdef.h to make checking easier
uint32_t keysymDead = 0;

struct keymap_t {
	uint32_t keysym;
	uint8_t vk;
	bool extended;
};

std::vector<uint8_t> deadChars;

static keymap_t keymap[] = {

	// TTY functions

	{ XK_BackSpace,        VK_BACK, 0 },
	{ XK_Tab,              VK_TAB, 0 },
	{ XK_Clear,            VK_CLEAR, 0 },
	{ XK_Return,           VK_RETURN, 0 },
	{ XK_Pause,            VK_PAUSE, 0 },
	{ XK_Escape,           VK_ESCAPE, 0 },
	{ XK_Delete,           VK_DELETE, 1 },

	// Japanese stuff - almost certainly wrong...
	//	[v1.0.2-jp1 fix] IOport's patch (Correct definition of Japanese key)
	//{ XK_Kanji,            VK_KANJI, 0 },
	//{ XK_Kana_Shift,       VK_KANA, 0 },
	// Japanese key
	{ XK_Kanji,            VK_KANJI, 0 },				/* 0x19: Kanji, Kanji convert */
	{ XK_Muhenkan,         VK_NONCONVERT, 0 },		/* 0x1d: Cancel Conversion */
	{ XK_Romaji,           VK_DBE_ROMAN, 0 },			/* 0xf5: to Romaji */
	{ XK_Hiragana,         VK_DBE_HIRAGANA, 0 },		/* 0xf2: to Hiragana */
	{ XK_Katakana,         VK_DBE_KATAKANA, 0 },		/* 0xf1: to Katakana */
	{ XK_Zenkaku,          VK_DBE_SBCSCHAR, 0 },		/* 0xf3: to Zenkaku */
	{ XK_Hankaku,          VK_DBE_DBCSCHAR, 0 },		/* 0xf4: to Hankaku */
	{ XK_Eisu_toggle,      VK_DBE_ALPHANUMERIC, 0 },	/* 0xf0: Alphanumeric toggle */
	{ XK_Mae_Koho,         VK_CONVERT, 0 },			/* 0x1c: Previous Candidate */

	// Cursor control & motion

	{ XK_Home,             VK_HOME, 1 },
	{ XK_Left,             VK_LEFT, 1 },
	{ XK_Up,               VK_UP, 1 },
	{ XK_Right,            VK_RIGHT, 1 },
	{ XK_Down,             VK_DOWN, 1 },
	{ XK_Page_Up,          VK_PRIOR, 1 },
	{ XK_Page_Down,        VK_NEXT, 1 },
	{ XK_End,              VK_END, 1 },

	// Misc functions

	{ XK_Select,           VK_SELECT, 0 },
	{ XK_Print,            VK_SNAPSHOT, 0 },
	{ XK_Execute,          VK_EXECUTE, 0 },
	{ XK_Insert,           VK_INSERT, 1 },
	{ XK_Help,             VK_HELP, 0 },
	{ XK_Break,            VK_CANCEL, 1 },

	// Keypad Functions, keypad numbers

	{ XK_KP_Space,         VK_SPACE, 0 },
	{ XK_KP_Tab,           VK_TAB, 0 },
	{ XK_KP_Enter,         VK_RETURN, 1 },
	{ XK_KP_F1,            VK_F1, 0 },
	{ XK_KP_F2,            VK_F2, 0 },
	{ XK_KP_F3,            VK_F3, 0 },
	{ XK_KP_F4,            VK_F4, 0 },
	{ XK_KP_Home,          VK_HOME, 0 },
	{ XK_KP_Left,          VK_LEFT, 0 },
	{ XK_KP_Up,            VK_UP, 0 },
	{ XK_KP_Right,         VK_RIGHT, 0 },
	{ XK_KP_Down,          VK_DOWN, 0 },
	{ XK_KP_End,           VK_END, 0 },
	{ XK_KP_Page_Up,       VK_PRIOR, 0 },
	{ XK_KP_Page_Down,     VK_NEXT, 0 },
	{ XK_KP_Begin,         VK_CLEAR, 0 },
	{ XK_KP_Insert,        VK_INSERT, 0 },
	{ XK_KP_Delete,        VK_DELETE, 0 },
	// XXX XK_KP_Equal should map in the same way as ascii '='
	{ XK_KP_Multiply,      VK_MULTIPLY, 0 },
	{ XK_KP_Add,           VK_ADD, 0 },
	{ XK_KP_Separator,     VK_SEPARATOR, 0 },
	{ XK_KP_Subtract,      VK_SUBTRACT, 0 },
	{ XK_KP_Decimal,       VK_DECIMAL, 0 },
	{ XK_KP_Divide,        VK_DIVIDE, 1 },

	{ XK_KP_0,             VK_NUMPAD0, 0 },
	{ XK_KP_1,             VK_NUMPAD1, 0 },
	{ XK_KP_2,             VK_NUMPAD2, 0 },
	{ XK_KP_3,             VK_NUMPAD3, 0 },
	{ XK_KP_4,             VK_NUMPAD4, 0 },
	{ XK_KP_5,             VK_NUMPAD5, 0 },
	{ XK_KP_6,             VK_NUMPAD6, 0 },
	{ XK_KP_7,             VK_NUMPAD7, 0 },
	{ XK_KP_8,             VK_NUMPAD8, 0 },
	{ XK_KP_9,             VK_NUMPAD9, 0 },

	// Auxilliary Functions

	{ XK_F1,               VK_F1, 0 },
	{ XK_F2,               VK_F2, 0 },
	{ XK_F3,               VK_F3, 0 },
	{ XK_F4,               VK_F4, 0 },
	{ XK_F5,               VK_F5, 0 },
	{ XK_F6,               VK_F6, 0 },
	{ XK_F7,               VK_F7, 0 },
	{ XK_F8,               VK_F8, 0 },
	{ XK_F9,               VK_F9, 0 },
	{ XK_F10,              VK_F10, 0 },
	{ XK_F11,              VK_F11, 0 },
	{ XK_F12,              VK_F12, 0 },
	{ XK_F13,              VK_F13, 0 },
	{ XK_F14,              VK_F14, 0 },
	{ XK_F15,              VK_F15, 0 },
	{ XK_F16,              VK_F16, 0 },
	{ XK_F17,              VK_F17, 0 },
	{ XK_F18,              VK_F18, 0 },
	{ XK_F19,              VK_F19, 0 },
	{ XK_F20,              VK_F20, 0 },
	{ XK_F21,              VK_F21, 0 },
	{ XK_F22,              VK_F22, 0 },
	{ XK_F23,              VK_F23, 0 },
	{ XK_F24,              VK_F24, 0 },

	// Modifiers

  { XK_Shift_L,          VK_SHIFT, 0 },
  { XK_Shift_R,          VK_RSHIFT, 0 },
  { XK_Control_L,        VK_CONTROL, 0 },
  { XK_Control_R,        VK_CONTROL, 1 },
  { XK_Alt_L,            VK_MENU, 0 },
  { XK_Alt_R,            VK_RMENU, 1 },

  // Left & Right Windows keys & Windows Menu Key

	{ XK_Super_L,			VK_LWIN, 0 },
	{ XK_Super_R,			VK_RWIN, 0 },
	{ XK_Menu,			VK_APPS, 0 },

};

struct latin1ToDeadChars_t {
	uint8_t latin1Char;
	uint8_t deadChar;
	uint8_t baseChar;
	int a, b, c;
};

latin1ToDeadChars_t latin1ToDeadChars[] = {

  { XK_Agrave, XK_grave, XK_A },
  { XK_Egrave, XK_grave, XK_E },
  { XK_Igrave, XK_grave, XK_I },
  { XK_Ograve, XK_grave, XK_O },
  { XK_Ugrave, XK_grave, XK_U },
  { XK_agrave, XK_grave, XK_a },
  { XK_egrave, XK_grave, XK_e },
  { XK_igrave, XK_grave, XK_i },
  { XK_ograve, XK_grave, XK_o},
  { XK_ugrave, XK_grave, XK_u },

  { XK_Aacute, XK_acute, XK_A },
  { XK_Eacute, XK_acute, XK_E },
  { XK_Iacute, XK_acute, XK_I },
  { XK_Oacute, XK_acute, XK_O },
  { XK_Uacute, XK_acute, XK_U },
  { XK_Yacute, XK_acute, XK_Y },
  { XK_aacute, XK_acute, XK_a },
  { XK_eacute, XK_acute, XK_e },
  { XK_iacute, XK_acute, XK_i },
  { XK_oacute, XK_acute, XK_o},
  { XK_uacute, XK_acute, XK_u },
  { XK_yacute, XK_acute, XK_y },

  { XK_Acircumflex, XK_asciicircum, XK_A },
  { XK_Ecircumflex, XK_asciicircum, XK_E },
  { XK_Icircumflex, XK_asciicircum, XK_I },
  { XK_Ocircumflex, XK_asciicircum, XK_O },
  { XK_Ucircumflex, XK_asciicircum, XK_U },
  { XK_acircumflex, XK_asciicircum, XK_a },
  { XK_ecircumflex, XK_asciicircum, XK_e },
  { XK_icircumflex, XK_asciicircum, XK_i },
  { XK_ocircumflex, XK_asciicircum, XK_o},
  { XK_ucircumflex, XK_asciicircum, XK_u },

  { XK_Adiaeresis, XK_diaeresis, XK_A },
  { XK_Ediaeresis, XK_diaeresis, XK_E },
  { XK_Idiaeresis, XK_diaeresis, XK_I },
  { XK_Odiaeresis, XK_diaeresis, XK_O },
  { XK_Udiaeresis, XK_diaeresis, XK_U },
  { XK_adiaeresis, XK_diaeresis, XK_a },
  { XK_ediaeresis, XK_diaeresis, XK_e },
  { XK_idiaeresis, XK_diaeresis, XK_i },
  { XK_odiaeresis, XK_diaeresis, XK_o},
  { XK_udiaeresis, XK_diaeresis, XK_u },
  { XK_ydiaeresis, XK_diaeresis, XK_y },

  { XK_Aring, XK_degree, XK_A },
  { XK_aring, XK_degree, XK_a },

  { XK_Ccedilla, XK_cedilla, XK_C },
  { XK_ccedilla, XK_cedilla, XK_c },

  { XK_Atilde, XK_asciitilde, XK_A },
  { XK_Ntilde, XK_asciitilde, XK_N },
  { XK_Otilde, XK_asciitilde, XK_O },
  { XK_atilde, XK_asciitilde, XK_a },
  { XK_ntilde, XK_asciitilde, XK_n },
  { XK_otilde, XK_asciitilde, XK_o },
};

uint8_t latin1DeadChars[] = {
  XK_grave, XK_acute, XK_asciicircum, XK_diaeresis, XK_degree, XK_cedilla,
  XK_asciitilde
};

#define NoSymbol 0xfff
unsigned short ascii_to_x[256] = {
	NoSymbol,	NoSymbol,	NoSymbol,	XK_KP_Enter,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_Delete,	XK_Tab,		XK_Linefeed,	NoSymbol,
	NoSymbol,	XK_Return,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	XK_Escape,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_space,	XK_exclam,	XK_quotedbl,	XK_numbersign,
	XK_dollar,	XK_percent,	XK_ampersand,	XK_apostrophe,
	XK_parenleft,	XK_parenright,	XK_asterisk,	XK_plus,
	XK_comma,	XK_minus,	XK_period,	XK_slash,
	XK_0,		XK_1,		XK_2,		XK_3,
	XK_4,		XK_5,		XK_6,		XK_7,
	XK_8,		XK_9,		XK_colon,	XK_semicolon,
	XK_less,	XK_equal,	XK_greater,	XK_question,
	XK_at,		XK_A,		XK_B,		XK_C,
	XK_D,		XK_E,		XK_F,		XK_G,
	XK_H,		XK_I,		XK_J,		XK_K,
	XK_L,		XK_M,		XK_N,		XK_O,
	XK_P,		XK_Q,		XK_R,		XK_S,
	XK_T,		XK_U,		XK_V,		XK_W,
	XK_X,		XK_Y,		XK_Z,		XK_bracketleft,
	XK_backslash,	XK_bracketright,XK_asciicircum,	XK_underscore,
	XK_grave,	XK_a,		XK_b,		XK_c,
	XK_d,		XK_e,		XK_f,		XK_g,
	XK_h,		XK_i,		XK_j,		XK_k,
	XK_l,		XK_m,		XK_n,		XK_o,
	XK_p,		XK_q,		XK_r,		XK_s,
	XK_t,		XK_u,		XK_v,		XK_w,
	XK_x,		XK_y,		XK_z,		XK_braceleft,
	XK_bar,		XK_braceright,	XK_asciitilde,	XK_BackSpace,
	// 128
		XK_Ccedilla,	XK_udiaeresis,	XK_eacute,	XK_acircumflex,
		XK_adiaeresis,	XK_agrave,	XK_aring,	XK_ccedilla,
		XK_ecircumflex,	XK_ediaeresis,	XK_egrave,	XK_idiaeresis,
		XK_icircumflex,	XK_igrave,	XK_Adiaeresis,	XK_Aring,
		XK_Eacute,	XK_ae,		XK_AE,		XK_ocircumflex,
		XK_odiaeresis,	XK_ograve,	XK_ntilde,	XK_ugrave,
		XK_ydiaeresis,	XK_Odiaeresis,	XK_Udiaeresis,	XK_cent,
		XK_sterling,	XK_yen,		XK_paragraph,	XK_section,
		// 160
			XK_aacute,	XK_degree,	XK_cent,	XK_sterling,
			XK_ntilde,	XK_Ntilde,	XK_paragraph,	XK_Greek_BETA,
			XK_questiondown,XK_hyphen,	XK_notsign,	XK_onehalf,
			XK_onequarter,	XK_exclamdown,	XK_guillemotleft,XK_guillemotright,
			NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
			NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
			NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
			NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
			// 192
				XK_questiondown,XK_exclamdown,	NoSymbol,	NoSymbol,
				NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
				NoSymbol,	NoSymbol,	NoSymbol,	XK_Agrave,
				NoSymbol,	NoSymbol,	XK_AE,		XK_ae,
				NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
				NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
				NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
				NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
				// 224
					XK_Greek_alpha,	XK_ssharp,	XK_Greek_GAMMA,	XK_Greek_pi,
					XK_Greek_SIGMA,	XK_Greek_sigma,	XK_mu,	        XK_Greek_tau,
					XK_Greek_PHI,	XK_Greek_THETA,	XK_Greek_OMEGA,	XK_Greek_delta,
					XK_infinity,	XK_Ooblique,	XK_Greek_epsilon, XK_intersection,
					XK_identical,	XK_plusminus,	XK_greaterthanequal, XK_lessthanequal,
					XK_topintegral,	XK_botintegral,	XK_division,	XK_similarequal,
					XK_degree,	NoSymbol,	NoSymbol,	XK_radical,
					XK_Greek_eta,	XK_twosuperior,	XK_periodcentered, NoSymbol,
};




// doKeyboardEvent wraps the system keybd_event function and attempts to find
// the appropriate scancode corresponding to the supplied virtual keycode.

inline void doKeyboardEvent(uint8_t vkCode, DWORD flags) {
	keybd_event(vkCode, MapVirtualKey(vkCode, 0), flags, 0);
}

// KeyStateModifier is a class which helps simplify generating a "fake" press
// or release of shift, ctrl, alt, etc.  An instance of the class is created
// for every key which may need to be pressed or released.  Then either press()
// or release() may be called to make sure that the corresponding key is in the
// right state.  The destructor of the class automatically reverts to the
// previous state.

class KeyStateModifier {
public:
	KeyStateModifier(int vkCode_, int flags_ = 0)
		: vkCode(vkCode_), 
		  flags(flags_), 
		  pressed(false), 
		  released(false)
	{}

	void press() {
		if (!(GetAsyncKeyState(vkCode) & 0x8000)) {
			doKeyboardEvent(vkCode, flags);
			pressed = true;
		}
	}

	void release() {
		if (GetAsyncKeyState(vkCode) & 0x8000) {
			doKeyboardEvent(vkCode, flags | KEYEVENTF_KEYUP);
			released = true;
		}
	}

	~KeyStateModifier() {
		if (pressed) {
			doKeyboardEvent(vkCode, flags | KEYEVENTF_KEYUP);
		}
		else if (released) {
			doKeyboardEvent(vkCode, flags);
		}
	}

public:
	int vkCode;
	int flags;
	bool pressed;
	bool released;
};

// Keymapper - a single instance of this class is used to generate Windows key
// events.
void doKeyEventWithModifiers(uint8_t vkCode, uint8_t modifierState, bool down)
{
	KeyStateModifier ctrl(VK_CONTROL);
	KeyStateModifier alt(VK_MENU);
	KeyStateModifier shift(VK_SHIFT);

	if (down) 
	{
		if (modifierState & 2) ctrl.press();
		if (modifierState & 4) alt.press();
		if (modifierState & 1) 
		{
			shift.press();
		}
		else 
		{
			shift.release();
		}
	}

	doKeyboardEvent(vkCode, down ? 0 : KEYEVENTF_KEYUP);
}

class Keymapper {

public:
	Keymapper()
	{
		for (int i = 0; i < sizeof(keymap) / sizeof(keymap_t); i++) 
		{
			vkMap[keymap[i].keysym] = keymap[i].vk;
			extendedMap[keymap[i].keysym] = keymap[i].extended;
		}

		// Find dead characters for the current keyboard layout

		uint8_t keystate[256];
		memset(keystate, 0, 256);
		for (int j = 0; j < sizeof(latin1DeadChars); j++) 
		{
			SHORT s = VkKeyScan(latin1DeadChars[j]);
			if (s != -1) 
			{
				uint8_t vkCode = LOBYTE(s);
				uint8_t modifierState = HIBYTE(s);
				keystate[VK_SHIFT] = (modifierState & 1) ? 0x80 : 0;
				keystate[VK_CONTROL] = (modifierState & 2) ? 0x80 : 0;
				keystate[VK_MENU] = (modifierState & 4) ? 0x80 : 0;
				uint8_t chars[2];
				int nchars = ToAscii(vkCode, 0, keystate, (WORD*)&chars, 0);
				if (nchars < 0) 
				{
					deadChars.push_back(latin1DeadChars[j]);
					ToAscii(vkCode, 0, keystate, (WORD*)&chars, 0);
				}
			}
		}

	}

	void keyEvent(uint32_t keysym, bool down, bool jap, bool unicode)
	{
		printf_s("recv keyEvent:%d, isDown:%d\n", keysym, down);
		Sleep(10);
		if (keysym >= XK_dead_grave && keysym <= XK_dead_belowdot)
		{
			keysymDead = keysym;
			printf_s(" ************** DEAD KEY\n");
			return;
		}

		if ((keysym >= 32 && keysym <= 126) ||
			(keysym >= 160 && keysym <= 255))
		{
			// printf_s("run into keysym judge\n");
			if (keysymDead != 0 && down)
			{
				printf_s(" Compose dead 0x%x 0x%x", keysymDead, keysym);
				switch (keysymDead)
				{
				case XK_dead_grave:
					switch (keysym)
					{
					case XK_A: keysym = XK_Agrave; break;
					case XK_E: keysym = XK_Egrave; break;
					case XK_I: keysym = XK_Igrave; break;
					case XK_O: keysym = XK_Ograve; break;
					case XK_U: keysym = XK_Ugrave; break;
					case XK_a: keysym = XK_agrave; break;
					case XK_e: keysym = XK_egrave; break;
					case XK_i: keysym = XK_igrave; break;
					case XK_o: keysym = XK_ograve; break;
					case XK_u: keysym = XK_ugrave; break;
					}
				case XK_dead_acute:
					switch (keysym)
					{
					case XK_A: keysym = XK_Aacute; break;
					case XK_E: keysym = XK_Eacute; break;
					case XK_I: keysym = XK_Iacute; break;
					case XK_O: keysym = XK_Oacute; break;
					case XK_U: keysym = XK_Uacute; break;
					case XK_a: keysym = XK_aacute; break;
					case XK_e: keysym = XK_eacute; break;
					case XK_i: keysym = XK_iacute; break;
					case XK_o: keysym = XK_oacute; break;
					case XK_u: keysym = XK_uacute; break;
					case XK_y: keysym = XK_yacute; break;
					case XK_Y: keysym = XK_Yacute; break;

					}
				case XK_dead_circumflex:
					switch (keysym)
					{
					case XK_A: keysym = XK_Acircumflex; break;
					case XK_E: keysym = XK_Ecircumflex; break;
					case XK_I: keysym = XK_Icircumflex; break;
					case XK_O: keysym = XK_Ocircumflex; break;
					case XK_U: keysym = XK_Ucircumflex; break;
					case XK_a: keysym = XK_acircumflex; break;
					case XK_e: keysym = XK_ecircumflex; break;
					case XK_i: keysym = XK_icircumflex; break;
					case XK_o: keysym = XK_ocircumflex; break;
					case XK_u: keysym = XK_ucircumflex; break;
					}
				case XK_dead_tilde:
					switch (keysym)
					{
					case XK_A: keysym = XK_Ntilde; break;
					case XK_O: keysym = XK_Otilde; break;
					case XK_a: keysym = XK_atilde; break;
					case XK_n: keysym = XK_ntilde; break;
					case XK_o: keysym = XK_otilde; break;
					}

				case XK_dead_diaeresis:
					switch (keysym)
					{
					case XK_A: keysym = XK_Adiaeresis; break;
					case XK_E: keysym = XK_Ediaeresis; break;
					case XK_I: keysym = XK_Idiaeresis; break;
					case XK_O: keysym = XK_Odiaeresis; break;
					case XK_U: keysym = XK_Udiaeresis; break;
					case XK_a: keysym = XK_adiaeresis; break;
					case XK_e: keysym = XK_ediaeresis; break;
					case XK_i: keysym = XK_idiaeresis; break;
					case XK_o: keysym = XK_odiaeresis; break;
					case XK_u: keysym = XK_udiaeresis; break;
					case XK_y: keysym = XK_ydiaeresis; break;

					}
				case XK_dead_cedilla:
					switch (keysym)
					{
					case XK_C: keysym = XK_Ccedilla; break;
					case XK_c: keysym = XK_ccedilla; break;
					}
				}
				keysymDead = 0;

			}
			// ordinary Latin-1 character
			SHORT s = VkKeyScan(keysym);

			if (s == -1)
			{

				if (down) 
				{
					// not a single keypress - try synthesizing dead chars.
					{
						//Lookup ascii representation
						int ascii = 0;

						ascii = keysym;
						if (ascii <= 255)
						{

							uint8_t a0 = ascii / 100;
							ascii = ascii % 100;
							uint8_t a1 = ascii / 10;
							ascii = ascii % 10;
							uint8_t a2 = ascii;

							KeyStateModifier shift(VK_SHIFT);
							KeyStateModifier lshift(VK_LSHIFT);
							KeyStateModifier rshift(VK_RSHIFT);

							lshift.release();
							rshift.release();

							printf_s(" Simulating ALT+%d%d%d\n", a0, a1, a2);

							keybd_event(VK_MENU, MapVirtualKey(VK_MENU, 0), 0, 0);
							/**
							  Pressing the Alt+NNN combinations without leading zero (for example, Alt+20, Alt+130, Alt+221)
							  will insert characters from the Extended ASCII (or MS DOS ASCII, or OEM) table. The character
							  glyphs contained by this table depend on the language of Windows. See the table below for the
							  list of characters that can be inserted through the Alt+NNN combinations (without leading zero)
							  in English Windows.

							  Pressing the Alt+0NNN combinations will insert the ANSI characters corresponding to the activate
							  keyboard layout. Please see Windows Character Map utility (charmap.exe) for the possible Alt+0NNN
							  combinations.

							  Finally, the Alt+00NNN combinations (two leading zeros) will insert Unicode characters. The Unicode
							  codes of characters are displayed in Charmap.

							**/
							// jdp 11 December 2008 - Need the leading 0! 
							keybd_event(VK_NUMPAD0, MapVirtualKey(VK_NUMPAD0, 0), 0, 0);
							keybd_event(VK_NUMPAD0, MapVirtualKey(VK_NUMPAD0, 0), KEYEVENTF_KEYUP, 0);
							keybd_event(VK_NUMPAD0 + a0, MapVirtualKey(VK_NUMPAD0 + a0, 0), 0, 0);
							keybd_event(VK_NUMPAD0 + a0, MapVirtualKey(VK_NUMPAD0 + a0, 0), KEYEVENTF_KEYUP, 0);
							keybd_event(VK_NUMPAD0 + a1, MapVirtualKey(VK_NUMPAD0 + a1, 0), 0, 0);
							keybd_event(VK_NUMPAD0 + a1, MapVirtualKey(VK_NUMPAD0 + a1, 0), KEYEVENTF_KEYUP, 0);
							keybd_event(VK_NUMPAD0 + a2, MapVirtualKey(VK_NUMPAD0 + a2, 0), 0, 0);
							keybd_event(VK_NUMPAD0 + a2, MapVirtualKey(VK_NUMPAD0 + a2, 0), KEYEVENTF_KEYUP, 0);
							keybd_event(VK_MENU, MapVirtualKey(VK_MENU, 0), KEYEVENTF_KEYUP, 0);
							return;
						}
					}
					printf_s("ignoring unrecognised Latin-1 keysym 0x%x", keysym);
				}
				return;
			}

			uint8_t vkCode = LOBYTE(s);

			// Correct the keymask shift state to cope with the capslock state
			BOOL capslockOn = (GetKeyState(VK_CAPITAL) & 1) != 0;

			uint8_t modifierState = HIBYTE(s);
			modifierState = capslockOn ? modifierState ^ 1 : modifierState;
			KeyStateModifier ctrl(VK_CONTROL);
			KeyStateModifier alt(VK_MENU);
			KeyStateModifier shift(VK_SHIFT);
			KeyStateModifier lshift(VK_LSHIFT);
			KeyStateModifier rshift(VK_RSHIFT);

			if (down) 
			{
				if (modifierState & 2) ctrl.press();
				if (modifierState & 4) alt.press();
				if (modifierState & 1) 
				{
					shift.press();
				}
				else {
					// [v1.0.2-jp1 fix] Even if "SHIFT + SPACE" are pressed, "SHIFT" is valid
					if (vkCode == 0x20) 
					{
						// nothing
					}
					else
					{
						lshift.release();
						rshift.release();
					}
				}
			}
			
			doKeyboardEvent(vkCode, down ? 0 : KEYEVENTF_KEYUP);

		}
		else {

			// see if it's a recognised keyboard key, otherwise ignore it

			if (vkMap.find(keysym) == vkMap.end()) 
			{
				if (unicode) 
				{
					if (keysym == 65509)
						return;
					INPUT inputs[1];
					char* key, text[32];
					sprintf_s(text, "%d", keysym);
					key = text;
					
					if (down) 
					{
						inputs[0].type = INPUT_KEYBOARD;
						inputs[0].ki.wVk = 0;
						inputs[0].ki.wScan = atoi(key);
						inputs[0].ki.time = 0;
						inputs[0].ki.dwExtraInfo = NULL;
						inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
						SendInput(1, inputs, sizeof(INPUT));
					}
					else 
					{
						inputs[0].type = INPUT_KEYBOARD;
						inputs[0].ki.wVk = 0;
						inputs[0].ki.wScan = atoi(key);
						inputs[0].ki.time = 0;
						inputs[0].ki.dwExtraInfo = NULL;
						inputs[0].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
						SendInput(1, inputs, sizeof(INPUT));
					}
				}
				return;
			}
			uint8_t vkCode = vkMap[keysym];
			DWORD flags = 0;
			if (extendedMap[keysym]) flags |= KEYEVENTF_EXTENDEDKEY;
			if (!down) flags |= KEYEVENTF_KEYUP;

			doKeyboardEvent(vkCode, flags);
		}
	}

private:
	std::map<uint32_t, uint8_t> vkMap;
	std::map<uint32_t, bool> extendedMap;
} key_mapper;

void vncKeymap::KeyEvent(unsigned int keysym, bool down, bool jap, bool unicode)
{
	key_mapper.keyEvent(keysym, down, jap, unicode);
}




void SetShiftState(uint8_t key, BOOL down)
{
	BOOL keystate = (GetAsyncKeyState(key) & 0x8000) != 0;

	// This routine sets the specified key to the desired value (up or down)
	if ((keystate && down) || ((!keystate) && (!down)))
		return;

	// Now send a key event to set the key to the new value
	doKeyboardEvent(key, down ? 0 : KEYEVENTF_KEYUP);
	keystate = (GetAsyncKeyState(key) & 0x8000) != 0;
}

void vncKeymap::ClearShiftKeys()
{
	if (0) // vncService::IsWinNT()
	{
		// On NT, clear both sets of keys

		// LEFT
		SetShiftState(VK_LSHIFT, FALSE);
		SetShiftState(VK_LCONTROL, FALSE);
		SetShiftState(VK_LMENU, FALSE);

		// RIGHT
		SetShiftState(VK_RSHIFT, FALSE);
		SetShiftState(VK_RCONTROL, FALSE);
		SetShiftState(VK_RMENU, FALSE);
	}
	else
	{
		// Otherwise, we can't distinguish the keys anyway...

		// Clear the shift key states
		SetShiftState(VK_SHIFT, FALSE);
		SetShiftState(VK_CONTROL, FALSE);
		SetShiftState(VK_MENU, FALSE);
	}
}
