/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifndef compzillaRenderingContextInternal_h___
#define compzillaRenderingContextInternal_h___


#ifdef MOZ_CAIRO_GFX
// Work around broken string APIs
typedef nsACString nsAFlatCString; 
#define nsString_h___ 1
#define nsAString_h___ 1
#define nsStringFwd_h___ 1
#include <nsIFrame.h> // unstable
#endif

#include <nsICanvasRenderingContextInternal.h> // unstable


// {fa62d345-f608-47eb-9401-533984cfd471}
#define COMPZILLA_RENDERING_CONTEXT_INTERNAL_IID \
    { 0xfa62d345, 0xf608, 0x47eb, { 0x94, 0x01, 0x53, 0x39, 0x84, 0xcf, 0xd4, 0x71 } }


class compzillaIRenderingContextInternal 
    : public nsICanvasRenderingContextInternal 
{
public:
#ifdef MOZILLA_1_8_BRANCH
    NS_DEFINE_STATIC_IID_ACCESSOR(COMPZILLA_RENDERING_CONTEXT_INTERNAL_IID)
#else
    NS_DECLARE_STATIC_IID_ACCESSOR(COMPZILLA_RENDERING_CONTEXT_INTERNAL_IID)
#endif

    NS_IMETHOD Redraw (nsRect rect) = 0;

    NS_IMETHOD SetDrawable (Display *dpy, Drawable drawable, Visual *visual) = 0;
};


#ifndef MOZILLA_1_8_BRANCH
NS_DEFINE_STATIC_IID_ACCESSOR(compzillaIRenderingContextInternal, 
                              COMPZILLA_RENDERING_CONTEXT_INTERNAL_IID)
#endif


#endif
