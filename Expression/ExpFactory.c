#include "ExpFactory.h"
#include <string.h>
#include "TxtExpr.h"
#include "NtButton.h"
#include "BrExpr.h"
#include "NtInput.h"
#include "GeometricsExpr.h"
#include "NtGrid.h"
#include "Dyn_TimeExpr.h"
#include "PicExpr.h"


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
	p = strstr( type, "box");
	if( p)
	{
		return (Expr *)GetGmtrExpr();
	}
	
	p = strstr( type, "rct");
	if( p)
	{
		return (Expr *)GetGmtrExpr();
	}
	p = strstr( type, "line");
	if( p)
	{
		return (Expr *)GetGmtrExpr();
	}
	p = strstr( type, "gr");
	if( p)
	{
		return (Expr *)GetNtGrid();
	}
	p = strstr( type, "pic");
	if( p)
	{
		return (Expr *)GetPictExpr();
	}
	
	
	//不注意大小写
	//动态显示图元
//	p = strstr( type, "time");
//	if( p)
//	{
//		return (Expr *)GetTimeExpr();
//	}
	
	if( !strcasecmp( type, "bu"))
	{
		return (Expr *)GetNtButton();
	}
//	
//	p = strstr( type, "br");
//	if( p)
//	{
//		return (Expr *)GetBrExpr();
//	}
	
	

//	err:		
	Except_raise(&Exp_Failed, __FILE__, __LINE__);
	
	return NULL;
	
}





