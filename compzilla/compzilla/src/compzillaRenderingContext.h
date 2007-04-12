/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#include "nsCOMPtr.h"

#define MOZILLA_INTERNAL_API
#include "nsICanvasRenderingContextInternal.h"
#undef MOZILLA_INTERNAL_API

#include "compzillaIRenderingContext.h"

#include "cairo.h"


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
     * Implement nsICanvasRenderingContextInternal and compzillaRenderingContextInternal
     */
    
    // This method should NOT hold a ref to aParentCanvas; it will be called
    // with nsnull when the element is going away.
    NS_IMETHOD SetCanvasElement(nsICanvasElement* aParentCanvas);
    
    // Sets the dimensions of the canvas, in pixels.  Called
    // whenever the size of the element changes.
    NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height);
    
    // Render the canvas at the origin of the given nsIRenderingContext
    NS_IMETHOD Render(nsIRenderingContext *rc);

    // Render the canvas at the origin of the given cairo surface
    NS_IMETHOD RenderToSurface(struct _cairo_surface *surf);

    // Gives you a stream containing the image represented by this context.
    // The format is given in aMimeTime, for example "image/png".
    //
    // If the image format does not support transparency or aIncludeTransparency
    // is false, alpha will be discarded and the result will be the image
    // composited on black.
    NS_IMETHOD GetInputStream(const nsACString& aMimeType,
                              const nsAString& aEncoderOptions,
                              nsIInputStream **aStream);



    NS_IMETHOD Redraw (nsRect rect);

    NS_IMETHOD SetDrawable (Display *dpy, Drawable drawable, Visual *visual);

private:
    nsIFrame* GetCanvasLayoutFrame ();
    
    nsICanvasElement* mCanvasElement;

    Display *mXDisplay;
    Visual *mXVisual;
    Pixmap mXDrawable;

    PRInt32 mWidth, mHeight;
};
