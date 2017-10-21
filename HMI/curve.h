#ifndef _INC_CURVE_H__
#define _INC_CURVE_H__
//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#include <stdint.h>
#include "HMI.h"
#include "commHMI.h"
//------------------------------------------------------------------------------
// check for correct compilation options
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
#define	CURVE_BEEK		5		//����ƽ����
//------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// global variable declarations
//------------------------------------------------------------------------------
extern sheet  		*g_p_curve_bkg;
//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------
void Curve_init(void);
void Curve_clean(curve_ctl_t *p_cctl);
void Curve_add_point(curve_ctl_t *p_cctl, int val);
void Curve_draw(curve_ctl_t *p_cctl);
void Curve_set(curve_ctl_t *p_cctl, int num, int clr, int start_x, int step);
#endif
