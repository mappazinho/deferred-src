#ifndef IDEF_COOKIE_H
#define IDEF_COOKIE_H

#include "cbase.h"
#include "materialsystem/itexture.h"

class IDefCookie
{
public:
	virtual ~IDefCookie(){};

	virtual ITexture *GetCookieTarget( const int iTargetIndex ) = 0;
	virtual void PreRender( const int iTargetIndex ){};

	virtual bool IsValid()
	{
		ITexture *pTexture = GetCookieTarget( 0 );
		return pTexture != NULL && !pTexture->IsError();
	};
};


#endif
