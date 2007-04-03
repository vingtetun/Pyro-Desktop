#define MOZILLA_INTERNAL_API

#include <X11/Xlib.h>

#include "prmem.h"
#include "prlog.h"

#include "nsIRenderingContext.h"

#include "compzillaIRenderingContext.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsIDOMHTMLCanvasElement.h"

#include "compzillaRenderingContext.h"

#ifdef MOZ_CAIRO_GFX
#include "thebes/gfxContext.h"
#include "thebes/gfxASurface.h"
#endif

// compzillaRenderingContext
// NS_DECL_CLASSINFO lives in nsCanvas3DModule
NS_IMPL_ADDREF(compzillaRenderingContext)
NS_IMPL_RELEASE(compzillaRenderingContext)

NS_IMPL_CI_INTERFACE_GETTER2(compzillaRenderingContext,
                             compzillaIRenderingContext,
                             nsICanvasRenderingContextInternal)

NS_INTERFACE_MAP_BEGIN(compzillaRenderingContext)
  NS_INTERFACE_MAP_ENTRY(compzillaIRenderingContext)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, compzillaIRenderingContext)
  NS_IMPL_QUERY_CLASSINFO(compzillaRenderingContext)
NS_INTERFACE_MAP_END

compzillaRenderingContext::compzillaRenderingContext()
{
}


compzillaRenderingContext::~compzillaRenderingContext()
{
}


NS_METHOD
compzillaRenderingContext::SetCanvasElement (nsICanvasElement* aParentCanvas)
{
    mCanvasElement = aParentCanvas;
    fprintf (stderr, "SetCanvasElement: %p\n", mCanvasElement);
    return NS_OK;
}


NS_METHOD 
compzillaRenderingContext::SetDimensions (PRInt32 width, PRInt32 height)
{
    fprintf (stderr, "SetDimensions\n");
    if (mWidth == width && mHeight == height)
        return NS_OK;

    // XXX do stuff

    //mSurface = new gfxGlitzSurface (mGlitzDrawable, gsurf, PR_TRUE);
    mWidth = width;
    mHeight = height;

    // allocate surface

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
  printf ("Render\n");

    nsresult rv = NS_OK;

    if (!mImageBuffer)
        return NS_OK;

#ifdef MOZ_CAIRO_GFX

    gfxContext* ctx = (gfxContext*) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
    nsRefPtr<gfxASurface> surf = gfxASurface::Wrap(mCairoImageSurface);
    nsRefPtr<gfxPattern> pat = new gfxPattern(surf);

    // XXX I don't want to use PixelSnapped here, but layout doesn't guarantee
    // pixel alignment for this stuff!
    ctx->NewPath();
    ctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, mWidth, mHeight), pat);
    ctx->Fill();

#else

    // non-Thebes; this becomes exciting
    cairo_surface_t *dest = nsnull;
    cairo_t *dest_cr = nsnull;

#ifdef XP_WIN
    void *ptr = nsnull;
#ifdef MOZILLA_1_8_BRANCH
    rv = rc->RetrieveCurrentNativeGraphicData(&ptr);
    if (NS_FAILED(rv) || !ptr)
        return NS_ERROR_FAILURE;
#else
    ptr = rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_WINDOWS_DC);
#endif
    HDC dc = (HDC) ptr;

    dest = cairo_win32_surface_create (dc);
    dest_cr = cairo_create (dest);
#endif

#ifdef MOZ_WIDGET_GTK2
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
#endif

    nsTransform2D *tx = nsnull;
    rc->GetCurrentTransform(tx);

    nsCOMPtr<nsIDeviceContext> dctx;
    rc->GetDeviceContext(*getter_AddRefs(dctx));

    // Until we can use the quartz2 surface, mac will be different,
    // since we'll use CG to render.
#ifndef XP_MACOSX

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

#else

    // OSX path

    CGrafPtr port = nsnull;
#ifdef MOZILLA_1_8_BRANCH
    rv = rc->RetrieveCurrentNativeGraphicData((void**) &port);
    if (NS_FAILED(rv) || !port)
        return NS_ERROR_FAILURE;
#else
    port = (CGrafPtr) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_MAC_THING);
    if (!port)
        return NS_ERROR_FAILURE;
#endif

    struct Rect portRect;
    GetPortBounds(port, &portRect);

    CGContextRef cgc;
    OSStatus status;
    status = QDBeginCGContext (port, &cgc);
    if (status != noErr)
        return NS_ERROR_FAILURE;

    CGDataProviderRef dataProvider;
    CGImageRef img;

    dataProvider = CGDataProviderCreateWithData (NULL, mImageBuffer,
                                                 mWidth * mHeight * 4,
                                                 NULL);
    CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
    img = CGImageCreate (mWidth, mHeight, 8, 32, mWidth * 4, rgb,
                         kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host,
                         dataProvider, NULL, false, kCGRenderingIntentDefault);
    CGColorSpaceRelease (rgb);
    CGDataProviderRelease (dataProvider);

    float x0 = 0.0, y0 = 0.0;
    float sx = 1.0, sy = 1.0;
    if (tx->GetType() & MG_2DTRANSLATION) {
        tx->Transform(&x0, &y0);
    }

    if (tx->GetType() & MG_2DSCALE) {
        float p2t = dctx->DevUnitsToTwips();
        sx = p2t, sy = p2t;
        tx->TransformNoXLate(&sx, &sy);
    }

    /* Compensate for the bottom-left Y origin */
    CGContextTranslateCTM (cgc, NSToIntRound(x0),
                           portRect.bottom - portRect.top - NSToIntRound(y0) - NSToIntRound(mHeight * sy));
    if (sx != 1.0 || sy != 1.0)
        CGContextScaleCTM (cgc, sx, sy);

    CGContextDrawImage (cgc, CGRectMake(0, 0, mWidth, mHeight), img);

    CGImageRelease (img);

    status = QDEndCGContext (port, &cgc);
    /* if EndCGContext fails, what can we do? */
#endif
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
compzillaRenderingContext::GetCanvas (nsIDOMHTMLCanvasElement **_retval)
{
    *_retval = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
compzillaRenderingContext::CopyImageDataFrom (PRUint32 disp_handle,
					      PRUint32 drawable,
					      PRInt32 src_x, PRInt32 src_y,
					      PRInt32 dest_x, PRInt32 dest_y,
					      PRUint32 w, PRUint32 h)
{
  printf ("CopyImageDataFrom\n");

  Display *display = (Display*)disp_handle;

  printf ("display = %p\n", display);

  XImage *image = XGetImage (display, drawable, src_x, src_y, w, h, AllPlanes, ZPixmap);

  printf ("yay\n");

#if false
#ifdef MOZ_CAIRO_GFX

    gfxContext* ctx = (gfxContext*) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
    nsRefPtr<gfxASurface> surf = gfxASurface::Wrap(mCairoImageSurface);
    nsRefPtr<gfxPattern> pat = new gfxPattern(surf);

    // XXX I don't want to use PixelSnapped here, but layout doesn't guarantee
    // pixel alignment for this stuff!
    ctx->NewPath();
    ctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, mWidth, mHeight), pat);
    ctx->Fill();

#else

    // non-Thebes; this becomes exciting
    cairo_surface_t *dest = nsnull;
    cairo_t *dest_cr = nsnull;

#ifdef MOZ_WIDGET_GTK2
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
#endif

    nsTransform2D *tx = nsnull;
    rc->GetCurrentTransform(tx);

    nsCOMPtr<nsIDeviceContext> dctx;
    rc->GetDeviceContext(*getter_AddRefs(dctx));

#endif

#if notyet
    nsresult rv;

    nsCOMPtr<nsIXPCNativeCallContext> ncc;
    rv = nsContentUtils::XPConnect()->
        GetCurrentNativeCallContext(getter_AddRefs(ncc));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!ncc)
        return NS_ERROR_FAILURE;

    PRUint32 argc;
    jsval *argv = nsnull;

    ncc->GetArgc(&argc);
    ncc->GetArgvPtr(&argv);

    JSObject *dataObject;
    int32 x, y;

    nsAutoArrayPtr<PRUint8> imageBuffer(new PRUint8[w * h * 4]);
    cairo_surface_t *imgsurf;
    PRUint8 *imgPtr = imageBuffer.get();
    jsval vr, vg, vb, va;
    PRUint8 ir, ig, ib, ia;
    for (int32 j = 0; j < h; j++) {
        for (int32 i = 0; i < w; i++) {
            if (!JS_GetElement(ctx, dataArray, (j*w*4) + i*4 + 0, &vr) ||
                !JS_GetElement(ctx, dataArray, (j*w*4) + i*4 + 1, &vg) ||
                !JS_GetElement(ctx, dataArray, (j*w*4) + i*4 + 2, &vb) ||
                !JS_GetElement(ctx, dataArray, (j*w*4) + i*4 + 3, &va))
                return NS_ERROR_DOM_SYNTAX_ERR;

#ifdef IS_LITTLE_ENDIAN
            *imgPtr++ = ib;
            *imgPtr++ = ig;
            *imgPtr++ = ir;
            *imgPtr++ = ia;
#else
            *imgPtr++ = ia;
            *imgPtr++ = ir;
            *imgPtr++ = ig;
            *imgPtr++ = ib;
#endif
        }
    }

    imgsurf = cairo_image_surface_create_for_data (imageBuffer.get(),
						   CAIRO_FORMAT_ARGB32,
						   w, h, w*4);
    cairo_save (mCairo);
    cairo_identity_matrix (mCairo);
    cairo_translate (mCairo, x, y);
    cairo_new_path (mCairo);
    cairo_rectangle (mCairo, 0, 0, w, h);
    cairo_set_source_surface (mCairo, imgsurf, 0, 0);
    cairo_set_operator (mCairo, CAIRO_OPERATOR_SOURCE);
    cairo_fill (mCairo);
    cairo_restore (mCairo);

    cairo_surface_destroy (imgsurf);
#endif
#endif

    return NS_OK;
}

nsresult
CZ_NewRenderingContext(compzillaIRenderingContext** aResult)
{
    compzillaIRenderingContext* ctx = new compzillaRenderingContext();
    if (!ctx)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = ctx);
    return NS_OK;
}
