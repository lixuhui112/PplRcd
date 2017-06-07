#include "testView.h"
#include "Composition.h"
#include "Compositor.h"
#include "Reader.h"
#include "ExpFactory.h"



#define  TEST_CONTEXT  4



#if TEST_CONTEXT == 1
static const char testContext[] = \
"<h1 c=red >12点阵全字库!</><h2 c=blue>16点阵全字库!</>\
<h3 c=yellow>2白日依山尽!</><h4 c=gren>黄河入海流流流</><h3>欲2穷千里目</><h6 c=purple>更上一层楼楼!</><h1 c=red >12点阵全字库!</>";
#elif TEST_CONTEXT == 2
static const char testContext[] = \
"<bu><h2 c=red >确认</></><bu><h2 c=red >返回</></>";
#elif TEST_CONTEXT == 3

static const char testContext[] = \
"<h2 c=red >组态</> <br/>\
<br/>\
<bu c=blue ><h2 c=red >系统组态</></bu><bu c=blue ><h2 c=red> 显示组态</></bu><br/>\
<bu c=blue ><h2 c=red >输入组态</></bu><bu c=blue ><h2 c=red> 输出组态</></bu><br/>\
<bu c=blue ><h2 c=red >记录组态</></bu><bu c=blue ><h2 c=red> 报警组态</></bu><br/>\
<bu c=blue ><h2 c=red >表报组态</></bu><bu c=blue ><h2 c=red> 打印组态</></bu><br/>\
<bu c=blue ><h2 c=red >通信组态</></bu><bu c=blue ><h2 c=red> 系统信息</></bu><br/>\
<br/>\
<bu c=yellow><h2 c=purple>组态文件</></bu><bu c=yellow><h2 c=purple>退出</></bu><br/>";
#elif TEST_CONTEXT == 4
static const char testContext[] = \
"<title bkc=black  f=24 ali=l>设置</> \
<input ali=m cg=2 id=0x01> <text f=24 clr=black >Passwd:</> <rct bkc=black x=96 y=30></></input>";

#endif

//背景色 "none" 或某种颜色
#define SCREENBKC		"white"
#define LINESPACING		2
#define COLUMGRAP		0

void View_test(void)
{
//	TestViewShow();
	char *pp = (void *)testContext;
	char	name[8];
	int		nameLen;
	Composition *ct = Get_Composition();
	Compositor *ctor = (Compositor *)Get_SimpCtor();
	Expr *myexp ;
	
	//设置排版的排版算法
	ct->lineSpacing = LINESPACING;
	ct->columnGap = COLUMGRAP;
	ct->setCtor( ct, ctor);
	ct->setSCBkc( ct, SCREENBKC);
	ct->clean( ct);
	

	
	while(1)
	{
		nameLen = 8;
		nameLen = GetName( pp, name, nameLen);
		if( nameLen == 0)
			break;
		
		myexp = ExpCreate( name);
		if( myexp == NULL)
			break;
		
			//设置排版
		myexp->setCtion( myexp, ct);
		myexp->setVar( myexp, name);		//跟据Context中的变量来设置
		pp = myexp->interpret( myexp, NULL, pp);
		
	}
		
	ct->flush( ct);
	

}



