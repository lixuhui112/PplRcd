#include "ExpFactory.h"
#include <string.h>
#include "TxtExpr.h"
#include "NtButton.h"
#include "BrExpr.h"
#include "NtInput.h"
#include "GeometricsExpr.h"

Expr *ExpCreate( char *type)
{
	char *p;
	
	p = strstr( type, "/");
	if( p)
	{
		
		return NULL;
	}
	
	p = strstr( type, "title");
	if( p)
	{
		return (Expr *)GetTxtExpr();
	}
	p = strstr( type, "text");
	if( p)
	{
		return (Expr *)GetTxtExpr();
	}
	
	p = strstr( type, "input");
	if( p)
	{
		return (Expr *)GetNtInput();
	}
	
	p = strstr( type, "rct");
	if( p)
	{
		return (Expr *)GetGmtrExpr();
	}
	
	
	
	//��ע���Сд
	
	
//	if( !strcasecmp( type, "h"))
//	{
//		return (Expr *)GetTxtExpr();
//	}
//	if( !strcasecmp( type, "h2"))
//	{
//		return (Expr *)GetTxtExpr();
//	}
//	if( !strcasecmp( type, "h3"))
//	{
//		return (Expr *)GetTxtExpr();
//	}
//	if( !strcasecmp( type, "h4"))
//	{
//		return (Expr *)GetTxtExpr();
//	}
//	if( !strcasecmp( type, "h5"))
//	{
//		return (Expr *)GetTxtExpr();
//	}
//	if( !strcasecmp( type, "h6"))
//	{
//		return (Expr *)GetTxtExpr();
//	}
//	if( !strcasecmp( type, "bu"))
//	{
//		return (Expr *)GetNtButton();
//	}
//	
//	p = strstr( type, "br");
//	if( p)
//	{
//		return (Expr *)GetBrExpr();
//	}
	
	
	
	return NULL;
	
}




