/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#define MOZILLA_INTERNAL_API

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "prmem.h"
#include "prlog.h"

#include "nsIRenderingContext.h"
#include "nsIViewManager.h"

#include "compzillaIRenderingContext.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsIDOMHTMLCanvasElement.h"

#include "compzillaRenderingContext.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#ifdef MOZ_CAIRO_GFX
#include "thebes/gfxContext.h"
#include "thebes/gfxASurface.h"
#include "thebes/gfxPlatform.h"
#include "thebes/gfxImageSurface.h"
#else
#include "nsTransform2D.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"
#endif


#define DEBUG(format...) printf("   - " format)
#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)


NS_IMPL_ISUPPORTS3_CI (compzillaRenderingContext,
		       compzillaIRenderingContext,
		       compzillaIRenderingContextInternal,
		       nsICanvasRenderingContextInternal)


compzillaRenderingContext::compzillaRenderingContext()
    : mWidth(0), 
      mHeight(0), 
      mStride(0), 
      mImageSurfaceData(nsnull),
      mCanvasElement(nsnull), 
      mSurface(nsnull), 
      mCairo(nsnull), 
      mSurfacePixmap(None)
{
}


compzillaRenderingContext::~compzillaRenderingContext()
{
}


NS_METHOD
compzillaRenderingContext::SetCanvasElement (nsICanvasElement* aParentCanvas)
{
    mCanvasElement = aParentCanvas;
    DEBUG ("SetCanvasElement: %p\n", mCanvasElement);
    return NS_OK;
}


void
compzillaRenderingContext::Destroy ()
{
    if (mCairo) {
        cairo_destroy(mCairo);
        mCairo = nsnull;
    }

    if (mSurface) {
      DEBUG ("destroying cairo surface");
      cairo_surface_destroy (mSurface);
      mSurface = nsnull;
    }

    if (mSurfacePixmap != None) {
        XFreePixmap(GDK_DISPLAY(), mSurfacePixmap);
        mSurfacePixmap = None;
    }

    if (mImageSurfaceData) {
        PR_Free (mImageSurfaceData);
        mImageSurfaceData = nsnull;
    }
}


NS_METHOD 
compzillaRenderingContext::SetDimensions (PRInt32 width, PRInt32 height)
{
    DEBUG ("SetDimensions (%d,%d)\n", width, height);

    Destroy();

#if false
    // Check that the dimensions are sane
    if (!CheckSaneImageSize(width, height))
        return NS_ERROR_FAILURE;
#endif

    mWidth = width;
    mHeight = height;

#ifdef MOZ_CAIRO_GFX
    DEBUG ("thebes\n");

    mThebesSurface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(
        gfxIntSize (width, height), 
        gfxASurface::ImageFormatARGB32);
    mThebesContext = new gfxContext(mThebesSurface);

    mSurface = mThebesSurface->CairoSurface();
    cairo_surface_reference(mSurface);
    mCairo = mThebesContext->GetCairo();
    cairo_reference(mCairo);

    DEBUG ("/thebes\n");
#else
    // non-cairo gfx
    // On most current X servers, using the software-only surface
    // actually provides a much smoother and faster display.
    // However, we provide MOZ_CANVAS_USE_RENDER for whomever wants to
    // go that route.

    //if (getenv("MOZ_CANVAS_USE_RENDER")) {
        XRenderPictFormat *fmt = XRenderFindStandardFormat (GDK_DISPLAY(),
                                                            PictStandardARGB32);
        if (fmt) {
            int npfmts = 0;
            XPixmapFormatValues *pfmts = XListPixmapFormats(GDK_DISPLAY(), &npfmts);
            for (int i = 0; i < npfmts; i++) {
                if (pfmts[i].depth == 32) {
                    npfmts = -1;
                    break;
                }
            }
            XFree(pfmts);

            if (npfmts == -1) {
                mSurfacePixmap = XCreatePixmap (GDK_DISPLAY(),
                                                DefaultRootWindow(GDK_DISPLAY()),
                                                width, height, 32);
                mSurface = cairo_xlib_surface_create_with_xrender_format
                    (GDK_DISPLAY(), mSurfacePixmap, DefaultScreenOfDisplay(GDK_DISPLAY()),
                     fmt, mWidth, mHeight);
            }
        }
    //}

    // fall back to image surface
    if (!mSurface) {
        mImageSurfaceData = (PRUint8*) PR_Malloc (mWidth * mHeight * 4);
        if (!mImageSurfaceData)
            return NS_ERROR_OUT_OF_MEMORY;

        mSurface = cairo_image_surface_create_for_data (mImageSurfaceData,
                                                        CAIRO_FORMAT_ARGB32,
                                                        mWidth, mHeight,
                                                        mWidth * 4);
    }

    mCairo = cairo_create(mSurface);
#endif

    cairo_set_operator(mCairo, CAIRO_OPERATOR_CLEAR);
    cairo_new_path(mCairo);
    cairo_rectangle(mCairo, 0, 0, mWidth, mHeight);
    cairo_fill(mCairo);

    cairo_set_line_width(mCairo, 1.0);
    cairo_set_operator(mCairo, CAIRO_OPERATOR_OVER);
    cairo_set_miter_limit(mCairo, 10.0);
    cairo_set_line_cap(mCairo, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(mCairo, CAIRO_LINE_JOIN_MITER);

    cairo_new_path(mCairo);

    return NS_OK;
}


nsIFrame*
compzillaRenderingContext::GetCanvasLayoutFrame()
{
    if (!mCanvasElement)
        return nsnull;

    nsIFrame *fr = nsnull;
    mCanvasElement->GetPrimaryCanvasFrame(&fr);
    return fr;
}


nsresult
compzillaRenderingContext::Redraw()
{
    // then invalidate the region and do a sync redraw
    // (uh, why sync?)
    nsIFrame *frame = GetCanvasLayoutFrame();
    if (frame) {
        nsRect r = frame->GetRect();
        r.x = r.y = 0;

        // sync redraw
        //frame->Invalidate(r, PR_TRUE);

        // nsIFrame::Invalidate is an internal non-virtual method,
        // so we basically recreate it here.  I would suggest
        // an InvalidateExternal for the trunk.
#if MOZILLA_1_8_BRANCH
        nsIPresShell *shell = frame->GetPresContext()->GetPresShell();
#else
        nsIPresShell *shell = frame->PresContext()->GetPresShell();
#endif
        if (shell) {
            PRBool suppressed = PR_FALSE;
            shell->IsPaintingSuppressed(&suppressed);
            if (suppressed)
                return NS_OK;
        }

        // maybe VMREFRESH_IMMEDIATE in some cases,
        // need to think
#if ASYNC_UPDATES
        PRUint32 flags = NS_VMREFRESH_NO_SYNC;
#else
        PRUint32 flags = NS_VMREFRESH_IMMEDIATE;
#endif
        if (frame->HasView()) {
            nsIView* view = frame->GetViewExternal();
            view->GetViewManager()->UpdateView(view, r, flags);
        } else {
            nsPoint offset;
            nsIView *view;
            frame->GetOffsetFromView(offset, &view);
            NS_ASSERTION(view, "no view");
            r += offset;
            view->GetViewManager()->UpdateView(view, r, flags);
        }
    }

    return NS_OK;
}


/*
 * This is identical to nsCanvasRenderingContext2D::Render, we just don't
 * have a good place to put it; though maybe I want a CanvasContextImpl that
 * all this stuff can derive from?
 */
NS_METHOD 
compzillaRenderingContext::Render (nsIRenderingContext *rc)
{
    DEBUG ("Render\n");

    nsresult rv = NS_OK;

#ifdef MOZ_CAIRO_GFX

    DEBUG ("thebes\n");

    gfxContext* ctx = (gfxContext*) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
    nsRefPtr<gfxPattern> pat = new gfxPattern(mThebesSurface);

    // XXX I don't want to use PixelSnapped here, but layout doesn't guarantee
    // pixel alignment for this stuff!
    ctx->NewPath();
    ctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, mWidth, mHeight), pat);
    ctx->Fill();
#else
    DEBUG ("non-thebes\n");

    // non-Thebes; this becomes exciting
    cairo_surface_t *dest = nsnull;
    cairo_t *dest_cr = nsnull;

    GdkDrawable *gdkdraw = nsnull;
#ifdef MOZILLA_1_8_BRANCH
    rv = rc->RetrieveCurrentNativeGraphicData((void**) &gdkdraw);
    if (NS_FAILED(rv) || !gdkdraw)
        return NS_ERROR_FAILURE;
#else
    gdkdraw = (GdkDrawable*) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_GDK_DRAWABLE);
    if (!gdkdraw)
        return NS_ERROR_FAILURE;
#endif

    gint w, h;
    gdk_drawable_get_size (gdkdraw, &w, &h);
    dest = cairo_xlib_surface_create (GDK_DRAWABLE_XDISPLAY(gdkdraw),
                                      GDK_DRAWABLE_XID(gdkdraw),
                                      GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(gdkdraw)),
                                      w, h);
    dest_cr = cairo_create (dest);

    nsTransform2D *tx = nsnull;
    rc->GetCurrentTransform(tx);

    nsCOMPtr<nsIDeviceContext> dctx;
    rc->GetDeviceContext(*getter_AddRefs(dctx));

    float x0 = 0.0, y0 = 0.0;
    float sx = 1.0, sy = 1.0;
    if (tx->GetType() & MG_2DTRANSLATION) {
        tx->Transform(&x0, &y0);
    }

    if (tx->GetType() & MG_2DSCALE) {
        sx = sy = dctx->DevUnitsToTwips();
        tx->TransformNoXLate(&sx, &sy);
    }

    cairo_translate (dest_cr, NSToIntRound(x0), NSToIntRound(y0));
    if (sx != 1.0 || sy != 1.0)
        cairo_scale (dest_cr, sx, sy);

    cairo_rectangle (dest_cr, 0, 0, mWidth, mHeight);
    cairo_clip (dest_cr);

    cairo_set_source_surface (dest_cr, mSurface, 0, 0);
    cairo_paint (dest_cr);

    if (dest_cr)
        cairo_destroy (dest_cr);
    if (dest)
        cairo_surface_destroy (dest);
#endif

    return rv;
}


NS_METHOD 
compzillaRenderingContext::RenderToSurface (struct _cairo_surface *surf)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD 
compzillaRenderingContext::GetInputStream (const nsACString& aMimeType,
					   const nsAString& aEncoderOptions,
					   nsIInputStream **aStream)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaRenderingContext::CopyImageDataFrom (Display *dpy,
					      Window drawable,
					      PRInt32 src_x, PRInt32 src_y,
					      PRUint32 w, PRUint32 h)
{
    DEBUG ("CopyImageDataFrom (%d,%d)\n", w, h);

    // XXX this is wrong, but it'll do for now.  it needs to create a
    // (possibly smaller) subimage and composite it into the larger
    // mSurface.

    XImage *image = XGetImage (dpy, drawable, src_x, src_y, w, h, AllPlanes, ZPixmap);

#ifdef MOZ_CAIRO_GFX
    DEBUG ("thebes\n");

    gfxImageSurface *imagesurf = new gfxImageSurface(gfxIntSize (w, h), 
						     gfxASurface::ImageFormatARGB32);

    unsigned char *r = imagesurf->Data();
    unsigned char *p = (unsigned char*)image->data;
    for (int i = 0; i < w * h; i ++) {
	*r++ = *p++;
	*r++ = *p++;
	*r++ = *p++;
	*r = 255; r++; p++;
    }

    mThebesContext->SetSource (imagesurf, gfxPoint (src_x, src_y));
    mThebesContext->Paint ();

    delete imagesurf;
#else
    // Fix colors
    unsigned char *r = (unsigned char*)image->data;
    for (int i = 0; i < w * h; i ++) {
      r[3] = 255; r += 4;
    }

    if (mImageSurfaceData) {
        int stride = mWidth*4;
        PRUint8 *dest = mImageSurfaceData + stride*src_y + src_x*4;

        for (int32 i = 0; i < src_y; i++) {
            memcpy(dest, image->data + (w*4)*i, w*4);
            dest += stride;
        }
    }
    else {
      cairo_surface_t *imgsurf;

        imgsurf = cairo_image_surface_create_for_data ((unsigned char*)image->data,
                                                       CAIRO_FORMAT_ARGB32,
                                                       w, h, w*4);
        cairo_save (mCairo);
        cairo_identity_matrix (mCairo);
        cairo_translate (mCairo, src_x, src_y);
        cairo_new_path (mCairo);
        cairo_rectangle (mCairo, 0, 0, w, h);
        cairo_set_source_surface (mCairo, imgsurf, 0, 0);
        cairo_set_operator (mCairo, CAIRO_OPERATOR_SOURCE);
        cairo_fill (mCairo);
        cairo_restore (mCairo);

        cairo_surface_destroy (imgsurf);
    }
#endif
    XFree (image->data);
    XFree (image);

    // XXX this redraws the entire canvas element.  we only need to
    // force a redraw of the affected rectangle.
    return Redraw ();
}

