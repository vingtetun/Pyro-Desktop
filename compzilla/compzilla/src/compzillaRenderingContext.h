/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifndef compzillaRenderingContext_h___
#define compzillaRenderingContext_h___


#include <nsCOMPtr.h>
#include <nsCycleCollectionParticipant.h>

#include <gfxContext.h>     // unstable
#include <gfxASurface.h>    // unstable
#include <gfxPlatform.h>    // unstable
#include <gfxXlibSurface.h> // unstable

extern "C" {
#include <X11/Xlib.h>
}

#include "compzillaIRenderingContext.h"
#include "compzillaIRenderingContextInternal.h"

#include <Layers.h>
typedef mozilla::layers::CanvasLayer CanvasLayer;
static PRUint8 gCompzillaLayerUserData;

class nsHTMLCanvasElement;

class compzillaRenderingContext :
    public compzillaIRenderingContext,
    public compzillaIRenderingContextInternal
{
public:
    NS_DECL_COMPZILLAIRENDERINGCONTEXT

    compzillaRenderingContext ();
    virtual ~compzillaRenderingContext();

    /*
     * Implement nsICanvasRenderingContextInternal
     */
    
    // This method should NOT hold a ref to aParentCanvas; it will be called
    // with nsnull when the element is going away.
    NS_IMETHOD SetCanvasElement(nsHTMLCanvasElement* aParentCanvas);
    
    // Sets the dimensions of the canvas, in pixels.  Called
    // whenever the size of the element changes.
    NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height);
    
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

    // Render the canvas at the origin of the given nsIRenderingContext
    NS_IMETHOD Render(gfxContext*, gfxPattern::GraphicsFilter);

    NS_IMETHOD Redraw (const gfxRect& rect);

    NS_IMETHOD SetDrawable (Display *dpy, Drawable drawable, Visual *visual);

    already_AddRefed<CanvasLayer> GetCanvasLayer(CanvasLayer *aOldLayer,
                                                 LayerManager *aManager) {
      if (!mValid)
        return nsnull;

      nsRefPtr<CanvasLayer> canvasLayer = aManager->CreateCanvasLayer();
      if (!canvasLayer) {
         NS_WARNING("CreateCanvasLayer returned null!");
        return nsnull;
      }
      canvasLayer->SetUserData(&gCompzillaLayerUserData, nsnull);

      CanvasLayer::Data data;
      data.mSurface = mGfxSurf.get();
      data.mSize = nsIntSize(mWidth, mHeight);
      canvasLayer->Initialize(data);

      PRUint32 flags = 0x01;
      canvasLayer->SetContentFlags(flags);
      canvasLayer->Updated(nsIntRect(0, 0, mWidth, mHeight));
 
      MarkContextClean();
      return canvasLayer.forget().get();
    };

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(compzillaRenderingContext, compzillaIRenderingContext)
private:
    nsHTMLCanvasElement* mCanvasElement;

    Display *mXDisplay;
    Visual *mXVisual;
    Pixmap mXDrawable;

    nsRefPtr<gfxXlibSurface> mGfxSurf;
    PRInt32 mWidth, mHeight;
    PRBool mValid;
};


#endif
