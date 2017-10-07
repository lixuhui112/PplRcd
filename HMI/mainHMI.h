#ifndef _INC_mainHMI_H_
#define _INC_mainHMI_H_
#include "HMI.h"
#include "commHMI.h"

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#include <stdint.h>
//------------------------------------------------------------------------------
// check for correct compilation options
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

 //------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------
CLASS( mainHmi)
{
	EXTENDS( HMI);
	
//	sheet  		*p_bkg;
//	sheet  		*p_title;
//	sheet  		*arr_p_sht_data[NUM_CHANNEL];
//	sheet  			**pp_shts;
//	uint8_t		focusRow;
//	uint8_t		focusCol;
//	uint8_t		none[2];
	
};
//------------------------------------------------------------------------------
// global variable declarations
//------------------------------------------------------------------------------
extern HMI *g_p_mainHmi;

//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------
mainHmi *Get_mainHmi(void) ;
#endif
