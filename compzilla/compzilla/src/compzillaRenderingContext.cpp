/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#define MOZILLA_INTERNAL_API

#include <prmem.h>
#include <prlog.h>

#include <nsIRenderingContext.h>
#include <nsIViewManager.h>

#include <nsICanvasRenderingContextInternal.h>
#include <nsIDOMHTMLCanvasElement.h>

#include "compzillaIRenderingContext.h"
#include "compzillaRenderingContext.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>


#if WITH_SPEW
#define SPEW(format...) printf("   - " format)
#else
#define SPEW(format...)
#endif

#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)

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
#ifdef MOZ_CAIRO_GFX
    // nsRefPtrs take care of initing to null
#else
    mCairoSurf = NULL;
#endif
}


compzillaRenderingContext::~compzillaRenderingContext ()
{
#ifdef MOZ_CAIRO_GFX
    // nsRefPtrs take care of cleaning up
#else
    if (mCairoSurf) {
        cairo_surface_destroy (mCairoSurf);
    }
#endif
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


nsIFrame*
compzillaRenderingContext::GetCanvasLayoutFrame ()
{
    nsIFrame *fr = nsnull;
    if (mCanvasElement) {
        mCanvasElement->GetPrimaryCanvasFrame(&fr);
    }
    return fr;
}


NS_IMETHODIMP
compzillaRenderingContext::Redraw (nsRect r)
{
    // then invalidate the region and do a sync redraw
    // (uh, why sync?)
    nsIFrame *frame = GetCanvasLayoutFrame ();
    if (frame) {
#if MOZILLA_1_8_BRANCH
        nsPresContext *presctx = frame->GetPresContext ();
        float p2t = presctx->PixelsToTwips ();
        /* 
         * 15 is the default for 96DPI:
         *   ((72 (point-to-twip) * 20 (twips-per-point)) * 96) 
         */
        r *= p2t ? p2t : 15.0;
#else
        nsPresContext *presctx = frame->PresContext ();
        r *= presctx->AppUnitsPerCSSPixel ();
#endif

        // sync redraw
        //frame->Invalidate(r, PR_TRUE);

        // nsIFrame::Invalidate is an internal non-virtual method,
        // so we basically recreate it here.  I would suggest
        // an InvalidateExternal for the trunk.
#ifndef DEBUG
        nsIPresShell *shell = presctx->GetPresShell ();
        if (shell) {
            PRBool suppressed = PR_FALSE;
            shell->IsPaintingSuppressed (&suppressed);
            if (suppressed)
                return NS_OK;
        }
#endif

        PRUint32 flags = NS_VMREFRESH_NO_SYNC;
#if SYNC_UPDATE
        // FIXME: Would be nice to force a sync update if this canvas is focused.
        flags = NS_VMREFRESH_IMMEDIATE;
#endif

        if (frame->HasView ()) {
            nsIView* view = frame->GetViewExternal ();
            view->GetViewManager ()->UpdateView (view, r, flags);
        } else {
            nsPoint offset;
            nsIView *view;

            frame->GetOffsetFromView (offset, &view);
            NS_ASSERTION (view, "no view");

            r += offset;
            view->GetViewManager ()->UpdateView (view, r, flags);
        }
    }

    return NS_OK;
}


/*
 * This is identical to nsCanvasRenderingContext2D::Render, we just don't
 * have a good place to put it; though maybe I want a CanvasContextImpl that
 * all this stuff can derive from?
 */
NS_IMETHODIMP
compzillaRenderingContext::Render (nsIRenderingContext *rc)
{
    nsresult rv = NS_OK;

#ifdef MOZ_CAIRO_GFX
    //    SPEW ("thebes\n");
    gfxContext* ctx = (gfxContext*) rc->GetNativeGraphicData (
        nsIRenderingContext::NATIVE_THEBES_CONTEXT);

    // XXX I don't want to use PixelSnapped here, but layout doesn't guarantee
    // pixel alignment for this stuff!
    ctx->NewPath ();
    ctx->PixelSnappedRectangleAndSetPattern (gfxRect(0, 0, mWidth, mHeight), mGfxPat);
    ctx->Fill ();
#else
    // non-Thebes; this becomes exciting

    GdkDrawable *gdkdraw = nsnull;
#ifdef MOZILLA_1_8_BRANCH
    rv = rc->RetrieveCurrentNativeGraphicData ((void**) &gdkdraw);
    if (NS_FAILED (rv) || !gdkdraw) {
        ERROR ("RetrieveCurrentNativeGraphicData failed.\n");
        return NS_ERROR_FAILURE;
    }
#else
    gdkdraw = (GdkDrawable*) rc->GetNativeGraphicData (
        nsIRenderingContext::NATIVE_GDK_DRAWABLE);
    if (!gdkdraw) {
        ERROR ("GetNativeGraphicData failed.\n");
        return NS_ERROR_FAILURE;
    }
#endif

    gint w, h;
    gdk_drawable_get_size (gdkdraw, &w, &h);

    cairo_surface_t *dest = cairo_xlib_surface_create (
        GDK_DRAWABLE_XDISPLAY (gdkdraw),
        GDK_DRAWABLE_XID (gdkdraw),
        GDK_VISUAL_XVISUAL (gdk_drawable_get_visual (gdkdraw)),
        w, h);

    cairo_t *dest_cr = cairo_create (dest);

    nsTransform2D *tx = nsnull;
    rc->GetCurrentTransform (tx);

    float x0 = 0.0, y0 = 0.0;
    float sx = 1.0, sy = 1.0;
    if (tx->GetType() & MG_2DTRANSLATION) {
        tx->Transform (&x0, &y0);
    }

    if (tx->GetType() & MG_2DSCALE) {
        nsIDeviceContext *dctx = nsnull;
        rc->GetDeviceContext (dctx);

        sx = sy = dctx->DevUnitsToTwips ();
        tx->TransformNoXLate (&sx, &sy);
    }

    cairo_translate (dest_cr, NSToIntRound(x0), NSToIntRound(y0));
    if (sx != 1.0 || sy != 1.0)
        cairo_scale (dest_cr, sx, sy);

    cairo_rectangle (dest_cr, 0, 0, mWidth, mHeight);
    cairo_clip (dest_cr);

    cairo_set_source_surface (dest_cr, mCairoSurf, 0, 0);
    cairo_paint (dest_cr);

    if (dest_cr)
        cairo_destroy (dest_cr);
    if (dest)
        cairo_surface_destroy (dest);
#endif

    return rv;
}


NS_IMETHODIMP
compzillaRenderingContext::RenderToSurface (struct _cairo_surface *surf)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaRenderingContext::GetInputStream (const nsACString& aMimeType,
					   const nsAString& aEncoderOptions,
					   nsIInputStream **aStream)
{
   return NS_ERROR_NOT_IMPLEMENTED;
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

#ifdef MOZ_CAIRO_GFX
    mGfxSurf = new gfxXlibSurface (mXDisplay, mXDrawable, mXVisual);
    mGfxPat = new gfxPattern (mGfxSurf);
#else
    if (mCairoSurf) {
        cairo_surface_destroy (mCairoSurf);
    }
    mCairoSurf = cairo_xlib_surface_create (mXDisplay, mXDrawable, mXVisual, mWidth, mHeight);
#endif

    return NS_OK;
}
