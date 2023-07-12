#include "KeyMap.h"
#include <tchar.h>
#include "ClientConnection.h"


// ModifierKeyReleaser is a class which helps simplify generating a "fake" release
//   of modifier key (shift, ctrl, alt, etc.) to server. An instance of the class is
//   created for every modifier key which may need to be released.
// Then either release() may be called to send key release event for KeySym that is
//   associated with that modifier key.
// The destructor of the class automatically send the key press event for the released
//   KeySym if release() was called during the existance of the instance.
class ModifierKeyReleaser {
public:
    ModifierKeyReleaser(ClientControlCaptureImpl* clientCon, uint8_t virtKey, bool extended)
        : process_callback_(clientCon), 
          extVkey(virtKey + (extended ? 256 : 0)), 
          keysym(0),
          modKeyName(nullptr)
    {
        switch (extVkey) {
        case VK_CONTROL:
        case VK_LCONTROL:
            modKeyName = _T("Left Ctrl ");
            break;
        case (VK_CONTROL + 256):
        case VK_RCONTROL:
            modKeyName = _T("Right Ctrl ");
            break;
        case VK_MENU:
        case VK_LMENU:
            modKeyName = _T("Left Alt ");
            break;
        case (VK_MENU + 256):
        case VK_RMENU:
            modKeyName = _T("Right Alt ");
            break;
        }
    }
    void release(uint32_t* downKeysym) 
    {
        if (downKeysym[extVkey] != XK_VoidSymbol) 
        {
            keysym = downKeysym[extVkey];
            process_callback_->SendKeyEvent(keysym, false);
        }
    }

    ~ModifierKeyReleaser() 
    {
        if (keysym) 
        {
            process_callback_->SendKeyEvent(keysym, true);
        }
    }

public:
    ClientControlCaptureImpl* process_callback_;
    UINT extVkey;
    uint32_t keysym;
    const TCHAR* modKeyName;
};


KeyMap::KeyMap(void* param)
    : storedDeadChar(0), 
      send_dead_key_(false), 
      use_unicode_keysym_(true),
      process_callback_((ClientControlCaptureImpl*)param)
{
    BuildUCS2KS_Map();
}

void KeyMap::SetSendDeadKey(bool sendDeadKeyOpt)
{
    send_dead_key_ = sendDeadKeyOpt;
    if (sendDeadKeyOpt)
    {
        storedDeadChar = 0;
    }
}

void KeyMap::SetUseUnicodeKeysym(bool useUnicodeKeysymOpt)
{
    use_unicode_keysym_ = useUnicodeKeysymOpt;
}

// Reset keyboard for first use or any other needs
void KeyMap::Reset()
{
    storedDeadChar = 0;
    GetKeyboardState(KBKeysState);
    FlushDeadKey(KBKeysState);

    for (int i = 0; i < (sizeof(downKeysym) / sizeof(uint32_t)); i++) {
        downKeysym[i] = XK_VoidSymbol;
    }

    for (int ii = 0; ii < (sizeof(downUnicode) / sizeof(WCHAR)); ii++) {
        downUnicode[ii] = NULL;
    }
}

// When losing focus the client should send key release to server for all sent key presses
void KeyMap::ReleaseAllKeys()
{
    for (int extVkey = 0; extVkey < (sizeof(downKeysym) / sizeof(uint32_t)); extVkey++) {
        if (downKeysym[extVkey] != XK_VoidSymbol) 
        {
            process_callback_->SendKeyEvent(downKeysym[extVkey], false);
            downKeysym[extVkey] = XK_VoidSymbol;
        }
    }
}

// Flush out the stored dead key 'state' for next usage
void KeyMap::FlushDeadKey(uint8_t* keyState)
{
    // To flush out the stored dead key 'state' we can not have any Alt key pressed
    keyState[VK_MENU] = keyState[VK_LMENU] = keyState[VK_RMENU] = 0;
    int ret = ToUnicode(VK_SPACE, 0, keyState, ucsChar, (sizeof(ucsChar) / sizeof(WCHAR)), 0);

    if (ret < 0) {
        FlushDeadKey(keyState);
    }
}

void KeyMap::StoreModifier(uint8_t* modifierState, uint8_t* keyState)
{
    if (keyState[VK_SHIFT] & 0x80) *modifierState |= 1;
    if (keyState[VK_CONTROL] & 0x80) *modifierState |= 2;
    if (keyState[VK_MENU] & 0x80) *modifierState |= 4;
}

void KeyMap::SetModifier(uint8_t modifierState, uint8_t* keyState)
{
    if (modifierState & 1) 
    {
        if ((keyState[VK_SHIFT] & 0x80) == 0) 
        {
            keyState[VK_SHIFT] = keyState[VK_LSHIFT] = 0x80;
            keyState[VK_RSHIFT] = 0;
        }
    }
    else 
    {
        if ((keyState[VK_SHIFT] & 0x80) != 0) 
        {
            keyState[VK_SHIFT] = keyState[VK_LSHIFT] = keyState[VK_RSHIFT] = 0;
        }
    }

    if (modifierState & 2) 
    {
        if ((keyState[VK_CONTROL] & 0x80) == 0) 
        {
            keyState[VK_CONTROL] = keyState[VK_LCONTROL] = 0x80;
            keyState[VK_RCONTROL] = 0;
        }
    }
    else 
    {
        if ((keyState[VK_CONTROL] & 0x80) != 0) 
        {
            keyState[VK_CONTROL] = keyState[VK_LCONTROL] = keyState[VK_RCONTROL] = 0;
        }
    }

    if (modifierState & 4) 
    {
        if ((keyState[VK_MENU] & 0x80) == 0) 
        {
            keyState[VK_MENU] = keyState[VK_LMENU] = 0x80;
            keyState[VK_RMENU] = 0;
        }
    }
    else 
    {
        if ((keyState[VK_MENU] & 0x80) != 0) 
        {
            keyState[VK_MENU] = keyState[VK_LMENU] = keyState[VK_RMENU] = 0;
        }
    }
}

void KeyMap::BuildUCS2KS_Map(void)
{
    int     i;
    uint32_t  j;
    WCHAR   ucs;

    // Building map for KeySym Latin-2, 3, 4, 8 & 9 and UCS Latin Extended-A & B
    for (i = 0, j = 0x01a1; i < (sizeof(keysym_to_unicode_1a1_1ff) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR)keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    for (i = 0, j = 0x02a1; i < (sizeof(keysym_to_unicode_2a1_2fe) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR)keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    for (i = 0, j = 0x03a2; i < (sizeof(keysym_to_unicode_3a2_3fe) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR)keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    for (i = 0, j = 0x12a1; i < (sizeof(keysym_to_unicode_12a1_12fe) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR)keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    for (i = 0, j = 0x13bc; i < (sizeof(keysym_to_unicode_13bc_13be) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR)keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Katakana and UCS Katakana, CJK Symbols and Punctuation
    for (i = 0, j = 0x04a1; i < (sizeof(keysym_to_unicode_4a1_4df) / sizeof(short)); i++, j++) {
        ksKatakanaMap[j] = ucs = (WCHAR)keysym_to_unicode_4a1_4df[i];
        if (IsUCSKatakana(ucs))
            ucsKatakanaMap[ucs] = j;
        else if (IsUCSCJKSym_Punc(ucs))
            ucsCJKSym_PuncMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Arabic and UCS Arabic
    for (i = 0, j = 0x0590; i < (sizeof(keysym_to_unicode_590_5fe) / sizeof(short)); i++, j++) {
        ksArabicMap[j] = ucs = (WCHAR)keysym_to_unicode_590_5fe[i];
        if (IsUCSArabic(ucs))
            ucsArabicMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Cyrillic and UCS Cyrillic
    for (i = 0, j = 0x0680; i < (sizeof(keysym_to_unicode_680_6ff) / sizeof(short)); i++, j++) {
        ksCyrillicMap[j] = ucs = (WCHAR)keysym_to_unicode_680_6ff[i];
        if (IsUCSCyrillic(ucs))
            ucsCyrillicMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Greek and UCS Greek and Coptic
    for (i = 0, j = 0x07a1; i < (sizeof(keysym_to_unicode_7a1_7f9) / sizeof(short)); i++, j++) {
        ksGreekMap[j] = ucs = (WCHAR)keysym_to_unicode_7a1_7f9[i];
        if (IsUCSGreek_Coptic(ucs))
            ucsGreek_CopticMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Technical and UCS Others
    for (i = 0, j = 0x08a4; i < (sizeof(keysym_to_unicode_8a4_8fe) / sizeof(short)); i++, j++) {
        ksTechnicalMap[j] = ucs = (WCHAR)keysym_to_unicode_8a4_8fe[i];
        if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Special and UCS Others
    for (i = 0, j = 0x09df; i < (sizeof(keysym_to_unicode_9df_9f8) / sizeof(short)); i++, j++) {
        ksSpecialMap[j] = ucs = (WCHAR)keysym_to_unicode_aa1_afe[i];
        if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Publishing and UCS Others
    for (i = 0, j = 0x0aa1; i < (sizeof(keysym_to_unicode_aa1_afe) / sizeof(short)); i++, j++) {
        ksPublishingMap[j] = ucs = (WCHAR)keysym_to_unicode_aa1_afe[i];
        if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Hebrew and UCS Hebrew, General Punctuation
    for (i = 0, j = 0x0cdf; i < (sizeof(keysym_to_unicode_cdf_cfa) / sizeof(short)); i++, j++) {
        ksHebrewMap[j] = ucs = (WCHAR)keysym_to_unicode_cdf_cfa[i];
        if (IsUCSHebrew(ucs))
            ucsHebrewMap[ucs] = j;
        else if (IsUCSGenPunc(ucs))
            ucsGenPuncMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Thai and UCS Thai
    for (i = 0, j = 0x0da1; i < (sizeof(keysym_to_unicode_da1_df9) / sizeof(short)); i++, j++) {
        ksThaiMap[j] = ucs = (WCHAR)keysym_to_unicode_da1_df9[i];
        if (IsUCSThai(ucs))
            ucsThaiMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Korean and UCS Hangul Jamo
    for (i = 0, j = 0x0ea0; i < (sizeof(keysym_to_unicode_ea0_eff) / sizeof(short)); i++, j++) {
        ksKoreanMap[j] = ucs = (WCHAR)keysym_to_unicode_ea0_eff[i];
        if (IsUCSHangulJamo(ucs))
            ucsHangulJamoMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Armenian and UCS Armenian, General Punctuation
    for (i = 0, j = 0x14a1; i < (sizeof(keysym_to_unicode_14a1_14ff) / sizeof(short)); i++, j++) {
        ksArmenianMap[j] = ucs = (WCHAR)keysym_to_unicode_14a1_14ff[i];
        if (IsUCSArmenian(ucs))
            ucsArmenianMap[ucs] = j;
        else if (IsUCSGenPunc(ucs))
            ucsGenPuncMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Georgian and UCS Georgian
    for (i = 0, j = 0x15d0; i < (sizeof(keysym_to_unicode_15d0_15f6) / sizeof(short)); i++, j++) {
        ksGeorgianMap[j] = ucs = (WCHAR)keysym_to_unicode_15d0_15f6[i];
        if (IsUCSGeorgian(ucs))
            ucsGeorgianMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Azeri and UCS Others
    for (i = 0, j = 0x16a0; i < (sizeof(keysym_to_unicode_16a0_16f6) / sizeof(short)); i++, j++) {
        ksAzeriMap[j] = ucs = (WCHAR)keysym_to_unicode_16a0_16f6[i];
        if (IsUCSLatinExtAdd(ucs))
            ucsLatinExtAddMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Vietnamese and UCS Latin Extended Additional, Latin Extended-A & B
    for (i = 0, j = 0x1e9f; i < (sizeof(keysym_to_unicode_1e9f_1eff) / sizeof(short)); i++, j++) {
        ksVietnameseMap[j] = ucs = (WCHAR)keysym_to_unicode_1e9f_1eff[i];
        if (IsUCSLatinExtAdd(ucs))
            ucsLatinExtAddMap[ucs] = j;
        else if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Currency and UCS Currency Symbols
    for (i = 0, j = 0x20a0; i < (sizeof(keysym_to_unicode_20a0_20ac) / sizeof(short)); i++, j++) {
        ksCurrencyMap[j] = ucs = (WCHAR)keysym_to_unicode_20a0_20ac[i];
        if (IsUCSCurrSym(ucs))
            ucsCurrSymMap[ucs] = j;
        else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }
}

uint32_t KeyMap::UCS2X(WCHAR UnicodeChar)
{
    uint32_t  XChar = XK_VoidSymbol;
    bool    isUnicodeKeysym = false;

    if (IsUCSPrintableLatin1(UnicodeChar)) 
    {
        // If the key is printable Latin-1 character
        // Latin-1 in UCS and X KeySym are exactly the same code
        XChar = (uint32_t)(UnicodeChar & 0xffff);
    }
    else if (UnicodeChar < 255) 
    {
        // Do not send non printable Latin-1 character
    }
    else if (IsUCSLatinExtAB(UnicodeChar)) 
    {
        // Latin Extended-A or B
        if (ucsLatinExtABMap.find(UnicodeChar) != ucsLatinExtABMap.end()) 
        {
            XChar = ucsLatinExtABMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSGreek_Coptic(UnicodeChar)) 
    {
        // Greek and Coptic
        if (ucsGreek_CopticMap.find(UnicodeChar) != ucsGreek_CopticMap.end()) 
        {
            XChar = ucsGreek_CopticMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSCyrillic(UnicodeChar)) 
    {
        // Cyrillic
        if (ucsCyrillicMap.find(UnicodeChar) != ucsCyrillicMap.end()) 
        {
            XChar = ucsCyrillicMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSArmenian(UnicodeChar)) 
    {
        // Armenian
        if (ucsArmenianMap.find(UnicodeChar) != ucsArmenianMap.end()) 
        {
            XChar = ucsArmenianMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSHebrew(UnicodeChar)) 
    {
        // Hebrew
        if (ucsHebrewMap.find(UnicodeChar) != ucsHebrewMap.end()) 
        {
            XChar = ucsHebrewMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSArabic(UnicodeChar)) 
    {
        // Arabic
        if (ucsArabicMap.find(UnicodeChar) != ucsArabicMap.end()) 
        {
            XChar = ucsArabicMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSThai(UnicodeChar)) 
    {
        // Thai
        if (ucsThaiMap.find(UnicodeChar) != ucsThaiMap.end()) 
        {
            XChar = ucsThaiMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSGeorgian(UnicodeChar)) 
    {
        // Georgian
        if (ucsGeorgianMap.find(UnicodeChar) != ucsGeorgianMap.end()) 
        {
            XChar = ucsGeorgianMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSHangulJamo(UnicodeChar)) 
    {
        // Hangul Jamo
        if (ucsHangulJamoMap.find(UnicodeChar) != ucsHangulJamoMap.end()) 
        {
            XChar = ucsHangulJamoMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSLatinExtAdd(UnicodeChar)) 
    {
        // Latin Extended Additional
        if (ucsLatinExtAddMap.find(UnicodeChar) != ucsLatinExtAddMap.end()) 
        {
            XChar = ucsLatinExtAddMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSGenPunc(UnicodeChar)) 
    {
        // General Punctuation
        if (ucsGenPuncMap.find(UnicodeChar) != ucsGenPuncMap.end()) 
        {
            XChar = ucsGenPuncMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSCurrSym(UnicodeChar)) 
    {
        // Currency Symbols
        if (ucsCurrSymMap.find(UnicodeChar) != ucsCurrSymMap.end()) 
        {
            XChar = ucsCurrSymMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSCJKSym_Punc(UnicodeChar)) 
    {
        // CJK Symbols and Punctuation
        if (ucsCJKSym_PuncMap.find(UnicodeChar) != ucsCJKSym_PuncMap.end()) 
        {
            XChar = ucsCJKSym_PuncMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (IsUCSKatakana(UnicodeChar)) 
    {
        // Katakana
        if (ucsKatakanaMap.find(UnicodeChar) != ucsKatakanaMap.end()) 
        {
            XChar = ucsKatakanaMap[UnicodeChar];
        }
        else if (use_unicode_keysym_) 
        {
            isUnicodeKeysym = true;
        }
    }
    else if (ucsOthersMap.find(UnicodeChar) != ucsOthersMap.end()) 
    {
        // Other Unicode Characters
        XChar = ucsOthersMap[UnicodeChar];
    }
    else if (use_unicode_keysym_) 
    {
        isUnicodeKeysym = true;
    }

    if (isUnicodeKeysym) {
         // Send the undefined X KeySym by using X protocol convention for
         //   defining new KeySym (Unicode KeySym):
         //   KeySym = ((UCS & 0xffffff) | 0x01000000)
         // Since Windows only support UCS-16 so we can have this formula:
        XChar = (((uint32_t)UnicodeChar) & 0xffff) | 0x01000000;
    }

    return XChar;
}

bool reset = false;

void KeyMap::ProcessKeyEvent(uint8_t virtKey, DWORD keyData)
{
    bool down = ((keyData & 0x80000000) == 0);
    bool extended = ((keyData & 0x1000000) != 0);
    bool repeated = ((keyData & 0xc0000000) == 0x40000000);
    UINT extVkey = virtKey + (extended ? 256 : 0);

    // exclude left and right winkey when not scroll-lock
    if (virtKey == 91 || virtKey == 92)
    {
        //return;
    }

    // key up event
    if (!down)
    {
        if (downUnicode[extVkey])
        {
            downUnicode[extVkey] = NULL;
        }
        
        ReleaseKey(extVkey);
        GetKeyboardState(KBKeysState);

        // get keyboard state, and judge key is down or not
        // KBKeysState[VK_MENU] & 0x80 != 0 indicate the key is down
        if (!((KBKeysState[VK_MENU] & 0x80) && (KBKeysState[VK_CONTROL] & 0x80)))
        {
            if (storedDeadChar && reset)
            {
                reset = false;
                keybd_event(VK_SPACE, 0, 0, 0);
                keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
            }
        }
        return;
    }

    // We try to look it up in our key table
    // Look up the desired code in the keyMap table try to find the exact match according to
    //   the extended flag, then try the opposite of the extended flag
    uint32_t foundXCode = XK_VoidSymbol;
    bool exactMatched = false;

    for (UINT i = 0; i < (sizeof(keyMap) / sizeof(vncKeyMapping_t)); i++)
    {
        if (keyMap[i].WinCode == virtKey)
        {
            foundXCode = keyMap[i].XCode;
            if (extended == keyMap[i].extVK)
            {
                exactMatched = true;
                break;
            }
        }
    }

    if (foundXCode != XK_VoidSymbol)
    {
        printf_s("-> keyMap gives (from %s extended flag) KeySym %u (0x%08x)\n",
                   (exactMatched ? _T("matched") : _T("opposite")),
                   foundXCode, foundXCode);
        PressKey(extVkey, foundXCode);
        return;
    }

    // Under CE, we're not so concerned about this bit because we handle a WM_CHAR message later
    GetKeyboardState(KBKeysState);

    ModifierKeyReleaser lctrl(process_callback_, VK_CONTROL, 0);
    ModifierKeyReleaser lalt(process_callback_, VK_MENU, 0);
    ModifierKeyReleaser ralt(process_callback_, VK_MENU, 1);

    if ((KBKeysState[VK_MENU] & 0x80) && (KBKeysState[VK_CONTROL] & 0x80)) {
        // This is a Ctrl-Alt (AltGr) key on international keyboards (= LCtrl-RAlt)
        // Ex. Ctrl-Alt-Q gives '@' on German keyboards

        // We must release Control and Alt (AltGr) if they were both pressed, so the character
        //   is seen without them by the VNC server
        // We don't release the Right Control; this allows German users
        //   to use it for doing Ctrl-AltGr-x, e.g. Ctl-@, etc
        lctrl.release(downKeysym);
        lalt.release(downKeysym);
        ralt.release(downKeysym);
    }
    else {
        // This is not a Ctrl-Alt (AltGr) key

        // There are no KeySym corresponding to control characters, e.g. Ctrl-F
        // The server has already known whether the Ctrl key is pressed from the previouse key event
        // So we are interested in the key that would be there if the Ctrl key were not pressed
        KBKeysState[VK_CONTROL] = KBKeysState[VK_LCONTROL] = KBKeysState[VK_RCONTROL] = 0;
    }

    int ret;
    if (storedDeadChar) {
        SHORT virtDeadKey;
        uint8_t prevModifierState = 0;

        StoreModifier(&prevModifierState, KBKeysState);
        virtDeadKey = VkKeyScanW(storedDeadChar);

        printf_s("[A dead key was stored, restoring the dead key state:", 
                 _T(" 0x%02x (%c) using virtDeadKey 0x%02x] "),
                 storedDeadChar, storedDeadChar, virtDeadKey);

        SetModifier(HIBYTE(virtDeadKey), KBKeysState);
        ToUnicode((virtDeadKey & 0xff), 0, KBKeysState, ucsChar, (sizeof(ucsChar) / sizeof(WCHAR)), 0);

        SetModifier(prevModifierState, KBKeysState);

        storedDeadChar = 0;
        ret = ToUnicode(virtKey, 0, KBKeysState, ucsChar, (sizeof(ucsChar) / sizeof(WCHAR)), 0);
    }
    else
    {
        ret = ToUnicode(virtKey, 0, KBKeysState, ucsChar, (sizeof(ucsChar) / sizeof(WCHAR)), 0);
    }

    if (ret < 0 || ret == 2) {
        //  It is a dead key

        if (send_dead_key_) {
            // We try to look it up in our dead key table
            // Look up the desired code in the deadKeyMap table
            foundXCode = XK_VoidSymbol;

            for (UINT i = 0; i < (sizeof(deadKeyMap) / sizeof(vncDeadKeyMapping_t)); i++) {
                if (deadKeyMap[i].deadKeyChar == *ucsChar) {
                    foundXCode = deadKeyMap[i].XCode;
                    break;
                }
            }

            if (foundXCode != XK_VoidSymbol) {
                printf_s("-> deadKeyMap gives KeySym %u (0x%08x)\n", foundXCode, foundXCode);
                PressKey(extVkey, foundXCode);
            }
        }
        else {
            storedDeadChar = *ucsChar;
            reset = true;
        }

        FlushDeadKey(KBKeysState);
    }
    else if (ret > 0) 
    {

        for (int i = 0; i < ret; i++)
        {
            uint32_t xChar = UCS2X(*(ucsChar + i));
            if (xChar != XK_VoidSymbol)
            {
                downUnicode[extVkey] = *(ucsChar + i);
                PressKey(extVkey, xChar);
            }
        }
    }
};

// This will send key release event to server if the VK key was previously pressed.
void KeyMap::ReleaseKey(UINT extVkey)
{
    if (downKeysym[extVkey] != XK_VoidSymbol) 
    {
        process_callback_->SendKeyEvent(downKeysym[extVkey], false);
        downKeysym[extVkey] = XK_VoidSymbol;
    }
}

// This will send key press event to server if the VK key was not previously pressed.
// The sent KeySym will be recorded so in can be retrieved later when this VK key is
//   released.
// However it is possible that the VK key has been pressed by other KeySym, then
//   we need to release the old KeySym first.
// One example is in a repeated key press event (auto repeat), the modifiers state
//   may differ from the previous key press event. If this is the case then it may
//   generate different character(s). Since the  server may have different keyboard
//   layout, this character(s) may be mapped to different KeyCap (ScanCode).
// Other example is if a single key press generates several characterss
void KeyMap::PressKey(UINT extVkey, uint32_t keysym)
{
    // release previous pressed down key, repeated key press event
    if (downKeysym[extVkey] != XK_VoidSymbol) {
        printf_s("The %ls VirtKey 0x%02x has been pressed -> Release previous associated KeySym 0x%08x\n",
                   ((extVkey & 256) ? _T("extended ") : _T("")),
                   (extVkey & 255), downKeysym[extVkey]);
        process_callback_->SendKeyEvent(downKeysym[extVkey], false);
        downKeysym[extVkey] = XK_VoidSymbol;
    }

    process_callback_->SendKeyEvent(keysym, true);
    downKeysym[extVkey] = keysym;
}
