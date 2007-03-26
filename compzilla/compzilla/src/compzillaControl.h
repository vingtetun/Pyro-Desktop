/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#include "compzillaIControl.h"
#include "nsCOMPtr.h"


class compzillaControl
    : public compzillaIControl
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAICONTROL

    compzillaControl();
    virtual ~compzillaControl();
};
