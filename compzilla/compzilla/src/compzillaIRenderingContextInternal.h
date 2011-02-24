/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifndef compzillaRenderingContextInternal_h___
#define compzillaRenderingContextInternal_h___

#include <nsRect.h>
#include <nsICanvasRenderingContextInternal.h> // unstable


// {fa62d345-f608-47eb-9401-533984cfd471}
#define COMPZILLA_RENDERING_CONTEXT_INTERNAL_IID \
    { 0xfa62d345, 0xf608, 0x47eb, { 0x94, 0x01, 0x53, 0x39, 0x84, 0xcf, 0xd4, 0x71 } }

class compzillaIRenderingContextInternal 
    : public nsICanvasRenderingContextInternal 
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(COMPZILLA_RENDERING_CONTEXT_INTERNAL_IID)

    NS_IMETHOD Render(gfxContext*, gfxPattern::GraphicsFilter) = 0;
    NS_METHOD InitializeWithSurface (nsIDocShell*, gfxASurface*, PRInt32, PRInt32) {};

    void MarkContextClean() { }

    NS_METHOD SetIsOpaque (PRBool b) { return NS_OK; };
    NS_METHOD Reset () { return NS_ERROR_NOT_IMPLEMENTED; };
    NS_IMETHOD SetIsIPC(PRBool b) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Redraw(const gfxRect&) = 0;

    NS_IMETHOD SetDrawable (Display *dpy, Drawable drawable, Visual *visual) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(compzillaIRenderingContextInternal, COMPZILLA_RENDERING_CONTEXT_INTERNAL_IID)
#endif
