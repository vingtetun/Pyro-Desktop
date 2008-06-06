/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifndef compzillaRenderingContext_h___
#define compzillaRenderingContext_h___


#include <nsCOMPtr.h>

#include <gfxContext.h>     // unstable
#include <gfxASurface.h>    // unstable
#include <gfxPlatform.h>    // unstable
#include <gfxXlibSurface.h> // unstable

extern "C" {
#include <X11/Xlib.h>
}

#include "compzillaIRenderingContext.h"
#include "compzillaIRenderingContextInternal.h"


class compzillaRenderingContext :
    public compzillaIRenderingContext,
    public compzillaIRenderingContextInternal
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAIRENDERINGCONTEXT

    compzillaRenderingContext ();
    virtual ~compzillaRenderingContext();

    /*
     * Implement nsICanvasRenderingContextInternal
     */
    
    // This method should NOT hold a ref to aParentCanvas; it will be called
    // with nsnull when the element is going away.
    NS_IMETHOD SetCanvasElement(nsICanvasElement* aParentCanvas);
    
    // Sets the dimensions of the canvas, in pixels.  Called
    // whenever the size of the element changes.
    NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height);
    
    // Render the canvas at the origin of the given nsIRenderingContext
    NS_IMETHOD Render(gfxContext *ctx);

    // Gives you a stream containing the image represented by this context.
    // The format is given in aMimeTime, for example "image/png".
    //
    // If the image format does not support transparency or aIncludeTransparency
    // is false, alpha will be discarded and the result will be the image
    // composited on black.
    NS_IMETHOD GetInputStream(const char *aMimeType,
                              const PRUnichar *aEncoderOptions,
                              nsIInputStream **aStream);

    // If this canvas context can be represented with a simple Thebes surface,
    // return the surface.  Otherwise returns an error.
    NS_IMETHOD GetThebesSurface(gfxASurface **surface);

    /*
     * Implement compzillaIRenderingContextInternal
     */

    NS_IMETHOD Redraw (nsRect rect);

    NS_IMETHOD SetDrawable (Display *dpy, Drawable drawable, Visual *visual);

private:
    nsICanvasElement* mCanvasElement;

    Display *mXDisplay;
    Visual *mXVisual;
    Pixmap mXDrawable;

    nsRefPtr<gfxXlibSurface> mGfxSurf;
    PRInt32 mWidth, mHeight;
};


#endif
