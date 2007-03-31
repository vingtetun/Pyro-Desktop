
#include "compzillaRenderingContext.h"


NS_IMPL_ISUPPORTS1(compzillaRenderingContext, nsICanvasRenderingContextInternal);


compzillaRenderingContext::compzillaRenderingContext()
{
}


compzillaRenderingContext::~compzillaRenderingContext()
{
}


NS_METHOD
compzillaRenderingContext::SetCanvasElement (nsICanvasElement* aParentCanvas)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD 
compzillaRenderingContext::SetDimensions (PRInt32 width, PRInt32 height)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD 
compzillaRenderingContext::Render (nsIRenderingContext *rc)
{
   return NS_ERROR_NOT_IMPLEMENTED;
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

