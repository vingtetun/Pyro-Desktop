/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#include "compzillaIControl.h"
#include "nsCOMPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

#include <gdk/gdkwindow.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>

#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 2
#define HAS_NAME_WINDOW_PIXMAP 1
#endif

class DictionaryEntry : public PLDHashEntryHdr
{
public:
    typedef const PRUint32& KeyType;
    typedef const PRUint32* KeyTypePointer;

    DictionaryEntry(KeyTypePointer aKey) : mValue(*aKey) { }
    DictionaryEntry(const DictionaryEntry& toCopy) : mValue(toCopy.mValue) { }
    ~DictionaryEntry() { }

    KeyType GetKey() const { return mValue; }
    KeyTypePointer GetKeyPointer() const { return &mValue; }
    PRBool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

    static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
    static PLDHashNumber HashKey(KeyTypePointer aKey) { return *aKey; }

    enum { ALLOW_MEMMOVE = PR_TRUE };

    nsCOMPtr<nsISupports> content;

private:
    const PRUint32 mValue;
};

class compzillaControl
    : public compzillaIControl
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAICONTROL

    compzillaControl();
    virtual ~compzillaControl();

private:
    void AddWindow (Window id, Window prev);

    GdkFilterReturn Filter (GdkXEvent *xevent, GdkEvent *event);
    static GdkFilterReturn gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data);

    GdkWindow *root;
    compzillaIWindowManager *wm;
    nsTHashtable<DictionaryEntry> *window_map;

    int		xfixes_event, xfixes_error;
    int		damage_event, damage_error;
    int		composite_event, composite_error;
    int		render_event, render_error;
    int		composite_opcode;

#if HAS_NAME_WINDOW_PIXMAP
    bool	hasNamePixmap;
#endif
};
