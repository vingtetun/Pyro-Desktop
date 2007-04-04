#define MOZILLA_INTERNAL_API

#include <X11/Xlib.h>

#include "prmem.h"
#include "prlog.h"

#include "nsIRenderingContext.h"
#include "nsIViewManager.h"

#include "compzillaIRenderingContext.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsIDOMHTMLCanvasElement.h"

#include "compzillaRenderingContext.h"

#ifdef MOZ_CAIRO_GFX
#include "thebes/gfxContext.h"
#include "thebes/gfxASurface.h"
#else
#include "nsTransform2D.h"
#include "cairo-xlib.h"
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#endif

#define DEBUG(format...) printf("   - " format)
#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)

NS_IMPL_ISUPPORTS3_CI (compzillaRenderingContext,
		       compzillaIRenderingContext,
		       compzillaIRenderingContextInternal,
		       nsICanvasRenderingContextInternal)

nsresult
CZ_NewRenderingContext(compzillaIRenderingContext** aResult)
{
    compzillaIRenderingContext* ctx = new compzillaRenderingContext();
    if (!ctx)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = ctx);
    return NS_OK;
}

compzillaRenderingContext::compzillaRenderingContext()
  : mWidth(0), mHeight(0), mStride(0), mImageBuffer(nsnull),
    mCanvasElement(nsnull),
    mCairoImageSurface(nsnull)
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
    if (mCairoImageSurface) {
      DEBUG ("destroying cairo surface");
      cairo_surface_destroy (mCairoImageSurface);
      mCairoImageSurface = nsnull;
    }
}

NS_METHOD 
compzillaRenderingContext::SetDimensions (PRInt32 width, PRInt32 height)
{
    DEBUG ("SetDimensions\n");

    if (mWidth == width && mHeight == height)
        return NS_OK;

    Destroy();

    int mStride = (((mWidth*4) + 3) / 4) * 4;

    mWidth = width;
    mHeight = height;

    // allocate surface
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
        nsIPresShell *shell = frame->GetPresContext()->GetPresShell();
        if (shell) {
            PRBool suppressed = PR_FALSE;
            shell->IsPaintingSuppressed(&suppressed);
            if (suppressed)
                return NS_OK;
        }

        // maybe VMREFRESH_IMMEDIATE in some cases,
        // need to think
        PRUint32 flags = NS_VMREFRESH_NO_SYNC;
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

   if (!mCairoImageSurface)
     return NS_OK;

    nsresult rv = NS_OK;

#ifdef MOZ_CAIRO_GFX

    DEBUG ("thebes\n");

    gfxContext* ctx = (gfxContext*) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);

    nsRefPtr<gfxASurface> surf = gfxASurface::Wrap(mCairoImageSurface);
    nsRefPtr<gfxPattern> pat = new gfxPattern(surf);

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

    cairo_set_source_surface (dest_cr, mCairoImageSurface, 0, 0);
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
   // XXX this is wrong, but it'll do for now.  it needs to create a
   // (possibly smaller) subimage and composite it into the larger
   // mCairoImageSurface.

   XImage *image = XGetImage (dpy, drawable, src_x, src_y, w, h, AllPlanes, ZPixmap);

   mImageBuffer = (PRUint8*)PR_MALLOC (image->bytes_per_line * image->height);

   // XXX we likely need to massage the data..
   memcpy (mImageBuffer, image->data, image->bytes_per_line * image->height);

   mCairoImageSurface = cairo_image_surface_create_for_data (mImageBuffer,
							     CAIRO_FORMAT_ARGB32,
							     w,
							     h,
							     image->bytes_per_line);

   XFree (image->data);
   XFree (image);

   // XXX this redraws the entire canvas element.  we only need to
   // force a redraw of the affected rectangle.
   return Redraw ();
}

