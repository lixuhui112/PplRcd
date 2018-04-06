#ifndef _INC_menuHMI_H_
#define _INC_menuHMI_H_
#include "HMI.h"

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
CLASS( menuHMI)
{
	EXTENDS( HMI);
//	IMPLEMENTS( shtCmd);
	sheet  			*p_sht_pic1;
	sheet  			*p_sht_pic2;

	uint8_t		focusRow;
	uint8_t		focusCol;
	uint8_t		none[2];
	
};
//------------------------------------------------------------------------------
// global variable declarations
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------
menuHMI *GetmenuHMI(void);
#endif
