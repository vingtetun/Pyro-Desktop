/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <prmem.h>
#include <prlog.h>

#include <nsMemory.h>
#include <nsIDeviceContext.h>        // unstable

#include <nsIDOMClassInfo.h>         // unstable
#include <nsIDOMHTMLCanvasElement.h> // unstable

#include "compzillaIRenderingContext.h"
#include "compzillaRenderingContext.h"
#include "Debug.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>


NS_IMPL_ISUPPORTS3_CI (compzillaRenderingContext,
		       compzillaIRenderingContext,
		       compzillaIRenderingContextInternal,
		       nsICanvasRenderingContextInternal)


compzillaRenderingContext::compzillaRenderingContext ()
    : mCanvasElement(NULL),
      mXDisplay(NULL),
      mXVisual(NULL),
      mXDrawable(None),
      mWidth(0), 
      mHeight(0)
{
    // nsRefPtrs take care of initing to null
}


compzillaRenderingContext::~compzillaRenderingContext ()
{
    // nsRefPtrs take care of cleaning up
}


NS_IMETHODIMP
compzillaRenderingContext::SetCanvasElement (nsICanvasElement* aParentCanvas)
{
    mCanvasElement = aParentCanvas;
    SPEW ("SetCanvasElement: %p\n", mCanvasElement);
    return NS_OK;
}


NS_IMETHODIMP
compzillaRenderingContext::SetDimensions (PRInt32 width, PRInt32 height)
{
    //    SPEW ("SetDimensions (%d,%d)\n", width, height);

#if false
    // Check that the dimensions are sane
    if (!CheckSaneImageSize (width, height))
        return NS_ERROR_FAILURE;
#endif

    mWidth = width;
    mHeight = height;

    return NS_OK;
}


NS_IMETHODIMP
compzillaRenderingContext::Redraw (nsRect r)
{
    //WARNING("Calling InvalidateFrameSubrect {x:%d, y:%d, w:%d, h:%d}\n",
    //        r.x, r.y, r.width, r.height);

    r *= nsIDeviceContext::AppUnitsPerCSSPixel();
    mCanvasElement->InvalidateFrameSubrect (r);
    return NS_OK;
}


/*
 * This is identical to nsCanvasRenderingContext2D::Render, we just don't
 * have a good place to put it; though maybe I want a CanvasContextImpl that
 * all this stuff can derive from?
 */
NS_IMETHODIMP
compzillaRenderingContext::Render (gfxContext *ctx)
{
    if (!mGfxSurf)
        return NS_ERROR_FAILURE;

    // FIXME: Not sure which of these two is better.
    //ctx->DrawSurface(mGfxSurf, gfxSize(mWidth, mHeight));

    nsRefPtr<gfxPattern> pat = new gfxPattern (mGfxSurf);
    ctx->NewPath ();
    ctx->PixelSnappedRectangleAndSetPattern (gfxRect(0, 0, mWidth, mHeight), pat);
    ctx->Fill ();

    return NS_OK;
}


NS_IMETHODIMP
compzillaRenderingContext::GetInputStream (const char *aMimeType,
                                           const PRUnichar *aEncoderOptions,
                                           nsIInputStream **aStream)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaRenderingContext::GetThebesSurface (gfxASurface **surface)
{
    *surface = mGfxSurf;
    return NS_OK;
}


/*
 * FIXME: This currently gets called before each Redraw, so do nothing if the
 *        values haven't changed.  This is broken.  SetDrawable should only be
 *        called when the backing pixmap gets recreated.  
 */
NS_IMETHODIMP
compzillaRenderingContext::SetDrawable (Display *dpy,
                                        Drawable drawable,
                                        Visual *visual)
{
    if (mXDisplay == dpy && mXDrawable == drawable && mXVisual == visual)
        return NS_OK;

    mXDisplay = dpy;
    mXDrawable = drawable;
    mXVisual = visual;

    mGfxSurf = new gfxXlibSurface (mXDisplay, mXDrawable, mXVisual, 
                                   gfxIntSize(mWidth, mHeight));

    return NS_OK;
}
