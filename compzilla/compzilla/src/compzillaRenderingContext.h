/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#include "nsCOMPtr.h"

#define MOZILLA_INTERNAL_API
#include "nsICanvasRenderingContextInternal.h"
#undef MOZILLA_INTERNAL_API

#include "compzillaIRenderingContext.h"

#include "cairo.h"

class compzillaRenderingContext :
    public compzillaIRenderingContext,
    public nsICanvasRenderingContextInternal
{
public:
  NS_DECL_ISUPPORTS

  NS_DECL_COMPZILLAIRENDERINGCONTEXT

  /*
   * Implement nsICanvasRenderingContextInternal...
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

  compzillaRenderingContext();

private:
  ~compzillaRenderingContext();

  nsICanvasElement* mCanvasElement;

  cairo_surface_t *mCairoImageSurface;
  PRUint8 *mImageBuffer;

  PRInt32 mWidth, mHeight, mStride;
};
