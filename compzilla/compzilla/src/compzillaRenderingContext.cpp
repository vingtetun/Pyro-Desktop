/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <prmem.h>
#include <prlog.h>

#include <nsMemory.h>
#include <nsIDeviceContext.h>        // unstable

#include <nsIDOMClassInfo.h>         // unstable
class nsHTMLCanvasElement;
#include <nsICanvasElementExternal.h> // unstable
#include <nsIDOMHTMLCanvasElement.h> // unstable

#include "compzillaIRenderingContext.h"
#include "compzillaRenderingContext.h"
#include "Debug.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

NS_IMPL_CLASSINFO(compzillaRenderingContext, NULL, 0, COMPZILLA_RENDERING_CONTEXT_CID)

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(compzillaRenderingContext, compzillaIRenderingContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(compzillaRenderingContext, compzillaIRenderingContext)

NS_IMPL_CYCLE_COLLECTION_CLASS(compzillaRenderingContext)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(compzillaRenderingContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCanvasElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(compzillaRenderingContext)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(compzillaRenderingContext)
  NS_INTERFACE_MAP_ENTRY(compzillaIRenderingContext)
  NS_INTERFACE_MAP_ENTRY(compzillaIRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, compzillaIRenderingContext)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(compzillaRenderingContext)
NS_INTERFACE_MAP_END


// Initialize our static variables.

nsresult
NS_NewCompzillaRenderingContext(compzillaIRenderingContext** aResult)
{
    printf("|||||| calling the builder\n");
    compzillaIRenderingContext* ctx = new compzillaRenderingContext();
    if (!ctx)
      return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = ctx);
    return NS_OK;
}

compzillaRenderingContext::compzillaRenderingContext ()
    : mCanvasElement(NULL),
      mXDisplay(NULL),
      mXVisual(NULL),
      mXDrawable(None),
      mWidth(0), 
      mHeight(0)
{
    printf("|||||| calling the constructor\n");
    // nsRefPtrs take care of initing to null
}

compzillaRenderingContext::~compzillaRenderingContext ()
{
    // nsRefPtrs take care of cleaning up
}

NS_IMETHODIMP
compzillaRenderingContext::SetCanvasElement (nsHTMLCanvasElement* aParentCanvas)
{
    mCanvasElement = aParentCanvas;
    SPEW ("SetCanvasElement: %p\n", mCanvasElement);
    return NS_OK;
}


NS_IMETHODIMP
compzillaRenderingContext::SetDimensions (PRInt32 width, PRInt32 height)
{
    SPEW ("SetDimensions (%d,%d)\n", width, height);

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
compzillaRenderingContext::Redraw (const gfxRect& r) {
    SPEW ("Redraw: %p\n", mCanvasElement);
    //WARNING("Calling InvalidateFrameSubrect {x:%d, y:%d, w:%d, h:%d}\n",
    //        r.x, r.y, r.width, r.height);

    gfxRect rect = r;
    rect.Scale(nsIDeviceContext::AppUnitsPerCSSPixel());
    nsCOMPtr<nsISupports> support = do_QueryInterface(reinterpret_cast<nsISupports*>(mCanvasElement));
    nsCOMPtr<nsICanvasElementExternal> p = do_QueryInterface(support);
    p->RedrawExternal(&rect);
    return NS_OK;
}


/*
 * This is identical to nsCanvasRenderingContext2D::Render, we just don't
 * have a good place to put it; though maybe I want a CanvasContextImpl that
 * all this stuff can derive from?
 */
NS_IMETHODIMP
compzillaRenderingContext::Render (gfxContext *ctx, gfxPattern::GraphicsFilter f) {
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
