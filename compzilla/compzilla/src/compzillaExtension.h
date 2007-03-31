/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#include "compzillaIExtension.h"
#include "nsCOMPtr.h"


class compzillaExtension : public compzillaIExtension
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_COMPZILLAIEXTENSION

  compzillaExtension();

private:
  ~compzillaExtension();
};
