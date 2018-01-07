#include "Model_channel.h"
#include <string.h>
#include <stdlib.h>
#include "mem/CiiMem.h"
#include "sdhDef.h"
#include "utils/Storage.h"
#include "Communication/smart_bus.h"
#include "device.h"
//通道模型:一个通道模型对应一个通道
//因此该模型在系统中会存在多个,不能使用单例
//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------

Model		*arr_p_mdl_chn[NUM_CHANNEL];
//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
/* Cycle/Sync Callback functions */
static int MdlChn_init( Model *self, IN void *arg);
static void MdlChn_run(Model *self);
static int MdlChn_self_check(Model *self);
static int MdlChn_getData(  Model *self, IN int aux, void *arg) ;
static int MdlChn_setData(  Model *self, IN int aux, void *arg) ;

static char* MdlChn_to_string( Model *self, IN int aux, void *arg);
static int  MdlChn_to_percentage( Model *self, void *arg);
static int MdlChn_set_by_string( Model *self, IN int aux, void *arg);
static int MdlChn_modify_sconf(Model *self, IN int aux, char *s, int op, int val);
static void Pe_singnaltype(e_signal_t sgt, char *str);
static void Pe_touch_spot(int spot, char *str);

static void MdlChn_Save_2_conf(mdl_chn_save_t *p_mcs, chn_info_t *p_cnf, uint8_t direct);

static void MdlChn_Save_2_alarm(mdl_chn_save_t *p_mcs, chn_alarm_t *p_alr, uint8_t direct);
//static int Str_to_data(char *str, int prec);
//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//
Model_chn *Get_Mode_chn(int n)
{
	Model_chn *p_mc = NULL;
	Model	*p_mdl;
	if(n > NUM_CHANNEL)
		return NULL;
	if(arr_p_mdl_chn[n])
		return SUB_PTR(arr_p_mdl_chn[n], Model, Model_chn);
	
	p_mc = Model_chn_new();
	p_mdl = SUPER_PTR(p_mc, Model);
	p_mdl->init(p_mdl, (void *)&n);
	arr_p_mdl_chn[n] = p_mdl;
	
	return p_mc;
}

CTOR( Model_chn)
SUPER_CTOR( Model);
FUNCTION_SETTING( Model.init, MdlChn_init);
FUNCTION_SETTING( Model.run, MdlChn_run);

FUNCTION_SETTING( Model.self_check, MdlChn_self_check);
FUNCTION_SETTING( Model.getMdlData, MdlChn_getData);
FUNCTION_SETTING( Model.setMdlData, MdlChn_setData);

FUNCTION_SETTING( Model.to_string, MdlChn_to_string);
FUNCTION_SETTING( Model.to_percentage, MdlChn_to_percentage);
FUNCTION_SETTING( Model.modify_str_conf, MdlChn_modify_sconf);

FUNCTION_SETTING( Model.set_by_string, MdlChn_set_by_string);

END_CTOR
//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{


static int MdlChn_init(Model *self, IN void *arg)
{
	int					chn_num = *((int *)arg);
	Model_chn		*cthis = SUB_PTR(self, Model, Model_chn);
	Storage			*stg = Get_storage();
	mdl_chn_save_t		save;
	
	cthis->str_buf = CALLOC(1,8);
	cthis->unit_buf = CALLOC(1,8);
	cthis->alarm_buf = CALLOC(1,8);
	self->mdl_id = MDLID_CHN(chn_num);
	cthis->chni.chn_NO = chn_num;
	
	if(stg->rd_stored_data(stg, CFG_CHN_CONF(cthis->chni.chn_NO), &save) != RET_OK) 
	{

		MdlChn_default_conf(chn_num);
		MdlChn_default_alarm(chn_num);
	}		
	else
	{
		MdlChn_Save_2_conf(&save, &cthis->chni, 0);
		MdlChn_Save_2_alarm(&save, &cthis->alarm, 0);
		
	}

	
	return RET_OK;
}

int	MdlChn_save_data(uint8_t chn_num, mdl_chn_save_t *p)
{
	
	Model_chn *p_mdl= Get_Mode_chn(chn_num);
	
	if(p_mdl == NULL)
		return 0;
	
	MdlChn_Save_2_conf(p, &p_mdl->chni, 1);
	MdlChn_Save_2_alarm(p, &p_mdl->alarm, 1);
	
	return (sizeof(p_mdl->chni) + sizeof(p_mdl->alarm));
}

void MdlChn_default_conf(int chn_num)
{
	Model_chn *p_mdl= Get_Mode_chn(chn_num);
	
	memset(&p_mdl->chni, 0, sizeof(p_mdl->chni));
	p_mdl->chni.signal_type = AI_0_400_ohm;
	p_mdl->chni.tag_NO = chn_num;
	p_mdl->chni.MB = 2;
}


void MdlChn_default_alarm(int chn_num)
{
	Model_chn *p_mdl= Get_Mode_chn(chn_num);
	
	memset(&p_mdl->alarm, 0, sizeof(p_mdl->alarm));
	p_mdl->alarm.alarm_hh = 0x7fff;
	p_mdl->alarm.alarm_hi = 0x7fff;
	
	
}

static int MdlChn_self_check( Model *self)
{
	Model_chn		*cthis = SUB_PTR(self, Model, Model_chn);
	uint8_t			chk_buf[32];
	I_dev_Char 		*I_uart3 = NULL;
	uint16_t		tmp_u16[2];
	uint8_t 			i, j;
	uint8_t				tmp_u8;
	
	Dev_open(DEVID_UART3, ( void *)&I_uart3);
	i = SmBus_Query(SMBUS_MAKE_CHN(SMBUS_CHN_AI, cthis->chni.chn_NO), chk_buf, 32);
	if( I_uart3->write(I_uart3, chk_buf, i) != RET_OK)
		return ERR_OPT_FAILED;
	i = I_uart3->read(I_uart3, chk_buf, 32);
	if(i <= 0)
		return ERR_OPT_FAILED;
	SmBus_decode(SMBUS_CMD_QUERY, chk_buf, &j, 1);
	if(j != cthis->chni.chn_NO)
		return ERR_OPT_FAILED;
	
	
	i = SmBus_rd_signal_type(SMBUS_MAKE_CHN(SMBUS_CHN_AI, cthis->chni.chn_NO), chk_buf, 32);
	if( I_uart3->write(I_uart3, chk_buf, i) != RET_OK)
		return ERR_OPT_FAILED;
	i = I_uart3->read(I_uart3, chk_buf, 32);
	if(i <= 0)
		return ERR_OPT_FAILED;
	if(SmBus_decode(SMBUS_CMD_READ, chk_buf, &tmp_u8, 1) != RET_OK)
		return ERR_OPT_FAILED;
	cthis->chni.signal_type = tmp_u8;
	
	
	i = SmBus_rd_h_l_limit(SMBUS_MAKE_CHN(SMBUS_CHN_AI, cthis->chni.chn_NO), chk_buf, 32);
	if( I_uart3->write(I_uart3, chk_buf, i) != RET_OK)
		return ERR_OPT_FAILED;
	i = I_uart3->read(I_uart3, chk_buf, 32);
	if(i <= 0)
		return ERR_OPT_FAILED;
	if(SmBus_decode(SMBUS_CMD_READ, chk_buf, tmp_u16, 4) != RET_OK)
		return ERR_OPT_FAILED;
	
	//todo：这段代码受到IOM模块的数据结构定义影响
	cthis->chni.upper_limit = tmp_u16[0];
	cthis->chni.lower_limit = tmp_u16[1];
	
	cthis->chni.decimal = 1;		//目前的smartbus的 工程值小数点只有1位
	return RET_OK;
	
}

static void MdlChn_run(Model *self)
{
	Model_chn			*cthis = SUB_PTR(self, Model, Model_chn);
	uint8_t				chk_buf[16];
	SmBus_result_t		rst;
	
	
	I_dev_Char 			*I_uart3 = NULL;
	uint8_t 			i;
	
	
	Dev_open(DEVID_UART3, ( void *)&I_uart3);
	i = SmBus_AI_Read(SMBUS_MAKE_CHN(SMBUS_CHN_AI, cthis->chni.chn_NO), chk_buf, 16);
	if( I_uart3->write(I_uart3, chk_buf, i) != RET_OK)
		return;
	i = I_uart3->read(I_uart3, chk_buf, 16);
	if(i <= 0)
		return;
	SmBus_decode(SMBUS_AI_READ, chk_buf, &rst, sizeof(SmBus_result_t));
	if(rst.chn_num != cthis->chni.chn_NO)
	{
		cthis->chni.flag_err = 1;
		return;
	}
	else
	{
		cthis->chni.flag_err = 0;
	}
		
	cthis->chni.signal_type = rst.signal_type;
	cthis->chni.value = rst.val;
	
	
}

static int MdlChn_getData(Model *self, IN int aux, void *arg) 
{
	Model_chn		*cthis = SUB_PTR( self, Model, Model_chn);
	int16_t			*p_s16;
	uint8_t			*p_u8;
	
	switch(aux) {
		case AUX_DATA:
			if(cthis->chni.flag_err)
				return -1;
			p_s16 = (int16_t *)arg;
			*p_s16 = cthis->chni.value;
			break;
		case AUX_SIGNALTYPE:
			p_u8 = (uint8_t *)arg;
			*p_u8 = cthis->chni.signal_type;
			break;
		case AUX_PERCENTAGE:
			
			return self->to_percentage(self, arg); 
		case MDHCHN_CHN_NUM:
			p_u8 = (uint8_t *)arg;
			*p_u8 = cthis->chni.chn_NO;
			break;
		default:
			break;
	}
		
	
	return RET_OK;
	
}



static int MdlChn_setData(  Model *self, IN int aux, void *arg) 
{
	Model_chn		*cthis = SUB_PTR( self, Model, Model_chn);
	uint8_t			*p_u8;
	SmBus_conf_t	sb_conf ;
	uint8_t			sbub_buf[32];
	I_dev_Char 			*I_uart3 = NULL;
	uint8_t 			i, tmp_u8;
	
	
	Dev_open(DEVID_UART3, ( void *)&I_uart3);
	switch(aux) {
		case AUX_SIGNALTYPE:
			p_u8 = (uint8_t *)arg;
			cthis->chni.signal_type = *p_u8;
		

		
			sb_conf.decimal = cthis->chni.decimal;
			sb_conf.lower_limit = cthis->chni.lower_limit;
			sb_conf.upper_limit = cthis->chni.upper_limit;
			i = SmBus_AI_config(SMBUS_MAKE_CHN(SMBUS_CHN_AI, cthis->chni.chn_NO), &sb_conf, sbub_buf, 32);
			if( I_uart3->write(I_uart3, sbub_buf, i) != RET_OK)
				return ERR_OPT_FAILED;
			i = I_uart3->read(I_uart3, sbub_buf, 32);
			if(i <= 0)
				return ERR_OPT_FAILED;
			if(SmBus_decode(SMBUS_AI_CONFIG, sbub_buf, &tmp_u8, 1) != RET_OK)
				return ERR_OPT_FAILED;
			if(tmp_u8 != cthis->chni.chn_NO)
				return ERR_OPT_FAILED;
			//回读检查
			
			i = SmBus_rd_signal_type(SMBUS_MAKE_CHN(SMBUS_CHN_AI, cthis->chni.chn_NO), sbub_buf, 32);
			if( I_uart3->write(I_uart3, sbub_buf, i) != RET_OK)
				return ERR_OPT_FAILED;
			i = I_uart3->read(I_uart3, sbub_buf, 32);
			if(i <= 0)
				return ERR_OPT_FAILED;
			if(SmBus_decode(SMBUS_CMD_READ, sbub_buf, &tmp_u8, 1) != RET_OK)
				return ERR_OPT_FAILED;
			if(cthis->chni.signal_type != tmp_u8)
			{
				cthis->chni.signal_type = tmp_u8;
				return ERR_OPT_FAILED;
			}
			
			return RET_OK;
		case AUX_PERCENTAGE:
			
			return self->to_percentage(self, arg); 
		default:
			break;
	}
		
	
	return ERR_OPT_ILLEGAL;
	
}

static char* MdlChn_to_string( Model *self, IN int aux, void *arg)
{
	Model_chn		*cthis = SUB_PTR( self, Model, Model_chn);
	char		*p = (char *)arg;
	int 			hh = cthis->chni.upper_limit;
	int 			hi = cthis->chni.upper_limit ;
	int 			li = cthis->chni.lower_limit ;
	int 			ll = 0;
	
	
	
	switch(aux) {
		case AUX_DATA:
			if( arg) {
				p = (char *)arg;		
			} else {
				p = cthis->str_buf;
			}

			sprintf( p, "%3d.%d", cthis->chni.value/10, cthis->chni.value%10);
			return p;
			

		case AUX_UNIT:
			if( arg) {
				p = (char *)arg;		
			} else {
				p = cthis->unit_buf;
			}
			Mdl_unit_to_string( cthis->chni.unit, p, 8);
			return p;
		case AUX_ALARM:
			if( arg) {
				p = (char *)arg;		
			} else {
				p = cthis->alarm_buf;
			}
		
			memset(p, 0, 8);
			if(cthis->chni.value > hh) {
				strcat(p, "HH ");
			}
			
			if(cthis->chni.value > hi) {
				strcat(p, "Hi ");
			}
			
			if(cthis->chni.value < li) {
				strcat(p, "Li ");
			}
			
			if(cthis->chni.value < ll) {
				strcat(p, "LL ");
			}
			
			//把空间填满，来达到清除掉上一次的显示的目的
			if( strlen( p) < 4) {
				strcat(p, "   ");
			}
			return p;
//		case AUX_PERCENTAGE:
//			if( arg) {
//				p = (char *)arg;		
//			} else {
//				p = cthis->str_buf;
//			}
//			if(cthis->range == 1000)
//				sprintf( p, "%d.%d", cthis->i_rand/10, cthis->i_rand%10);
//			else
//				sprintf( p, "%d", cthis->i_rand);
//			return p;
			
		case AUX_SIGNALTYPE:
			Pe_singnaltype((e_signal_t)cthis->chni.signal_type, (char *)arg);
			break;
		case chnaux_record_mb:
			sprintf(arg, "%d M", cthis->chni.MB);
			break;
		case chnaux_filter_ts:
			sprintf(arg, "%d S", cthis->chni.filter_time_s);
			break;
		case chnaux_lower_limit:
			Pe_float(cthis->chni.lower_limit, 1, (char *)arg);
			break;
		case chnaux_upper_limit:
			Pe_float(cthis->chni.upper_limit, 1, (char *)arg);
			break;
		case chnaux_small_signal:
			Pe_float(cthis->chni.small_signal, 1, (char *)arg);
			strcat((char *)arg, " %");
			break;
		case chnaux_k:
//			Pe_frefix_float(cthis->chni.k, 2, "K:",(char *)arg);
			Pe_float(cthis->chni.k, 2, (char *)arg);
			break;
		case chnaux_b:
//			Pe_frefix_float(cthis->chni.b, 1, "B:", (char *)arg);
			Pe_float(cthis->chni.b, 1, (char *)arg);
			break;	
	
		case alarm_hh:
			Pe_float(cthis->alarm.alarm_hh, 1, (char *)arg);
			break;
		case alarm_hi:
			Pe_float(cthis->alarm.alarm_hi, 1, (char *)arg);
			break;
		case alarm_lo:
			Pe_float(cthis->alarm.alarm_lo, 1, (char *)arg);
			break;
		case alarm_ll:
			Pe_float(cthis->alarm.alarm_ll, 1, (char *)arg);
			break;
		case tchspt_hh:
			Pe_touch_spot(cthis->alarm.touch_spot_hh, (char *)arg);
			break;
		case tchspt_hi:
			Pe_touch_spot(cthis->alarm.touch_spot_hi, (char *)arg);
			break;
		case tchspt_lo:
			Pe_touch_spot(cthis->alarm.touch_spot_lo, (char *)arg);	
			break;
		case tchspt_ll:
			Pe_touch_spot(cthis->alarm.touch_spot_ll, (char *)arg);	
			break;	
		case alarm_backlash:
			Pe_float(cthis->alarm.alarm_backlash, 1, (char *)arg);
			break;
		default:
			break;
			
		
	}
	return NULL;
}
static int MdlChn_modify_sconf(Model *self, IN int aux, char *s, int op, int val)
{
	Model_chn		*cthis = SUB_PTR( self, Model, Model_chn);
	phn_sys.save_chg_flga |= CHG_MODCHN_CONF(cthis->chni.chn_NO);
	switch(aux) {
		case AUX_UNIT:
			cthis->chni.unit = Operate_in_tange(cthis->chni.unit, op, 1, 0, eu_max - 1);
			self->to_string(self, AUX_UNIT, s);
			break;
		case chnaux_tag_NO:
			cthis->chni.tag_NO = Operate_in_tange(cthis->chni.tag_NO, op, 1, 0, 9);
			sprintf(s, "%d", cthis->chni.tag_NO);
			break;
		case AUX_SIGNALTYPE:
			cthis->chni.signal_type = Operate_in_tange(cthis->chni.signal_type, op, 1, 0, es_max - 1);
			self->to_string(self, AUX_SIGNALTYPE, s);
			break;
		case chnaux_record_mb:
			cthis->chni.MB = Operate_in_tange(cthis->chni.MB, op, val, 0, 99);
			
			sprintf(s, "%d M", cthis->chni.MB);
			break;
		case chnaux_filter_ts:
			cthis->chni.filter_time_s = Operate_in_tange(cthis->chni.filter_time_s, op, val, 0, 99);
			
			sprintf(s, "%d S", cthis->chni.filter_time_s);
			break;
		case chnaux_lower_limit:
			cthis->chni.lower_limit = Operate_in_tange(cthis->chni.lower_limit, op, val, -999999, 999999);
			self->to_string(self, chnaux_lower_limit, s);
			break;
		case chnaux_upper_limit:
			cthis->chni.upper_limit = Operate_in_tange(cthis->chni.upper_limit, op, val, -999999, 999999);
			self->to_string(self, chnaux_upper_limit, s);
			break;
		case chnaux_small_signal:
			cthis->chni.small_signal = Operate_in_tange(cthis->chni.small_signal, op, val, 0, 100);
			self->to_string(self, chnaux_small_signal, s);
			break;
		case chnaux_k:
			cthis->chni.k = Operate_in_tange(cthis->chni.k, op, val, -100, 100);
			self->to_string(self, chnaux_k, s);
			break;
		case chnaux_b:
			cthis->chni.b = Operate_in_tange(cthis->chni.b, op, val, -100, 100);
			self->to_string(self, chnaux_b, s);
			break;
		
		case alarm_hh:
			cthis->alarm.alarm_hh = Operate_in_tange(cthis->alarm.alarm_hh, op, val, -100, 100);
			Pe_float(cthis->alarm.alarm_hh, 1, s);
			break;
		case alarm_hi:
			cthis->alarm.alarm_hi = Operate_in_tange(cthis->alarm.alarm_hi, op, val, -100, 100);
			Pe_float(cthis->alarm.alarm_hi, 1, s);
			break;
		case alarm_lo:
			cthis->alarm.alarm_lo = Operate_in_tange(cthis->alarm.alarm_lo, op, val, -100, 100);
			Pe_float(cthis->alarm.alarm_lo, 1, s);
			break;
		case alarm_ll:
			cthis->alarm.alarm_ll = Operate_in_tange(cthis->alarm.alarm_ll, op, val, -100, 100);
			Pe_float(cthis->alarm.alarm_ll, 1, (char *)s);
			break;
		case tchspt_hh:
			cthis->alarm.touch_spot_hh = Operate_in_tange(cthis->alarm.touch_spot_hh, op, val, 1, 13);
			Pe_touch_spot(cthis->alarm.touch_spot_hh, (char *)s);
			break;
		case tchspt_hi:
			cthis->alarm.touch_spot_hi = Operate_in_tange(cthis->alarm.touch_spot_hi, op, val, 1, 13);
			Pe_touch_spot(cthis->alarm.touch_spot_hi, (char *)s);
			break;
		case tchspt_lo:
			cthis->alarm.touch_spot_lo = Operate_in_tange(cthis->alarm.touch_spot_lo, op, val, 1, 13);
			Pe_touch_spot(cthis->alarm.touch_spot_lo, (char *)s);	
			break;
		case tchspt_ll:
			cthis->alarm.touch_spot_ll = Operate_in_tange(cthis->alarm.touch_spot_ll, op, val, 1, 13);
			Pe_touch_spot(cthis->alarm.touch_spot_ll, (char *)s);	
			break;	
		case alarm_backlash:
			cthis->alarm.alarm_backlash = Operate_in_tange(cthis->alarm.alarm_backlash, op, val, 0, 100);
			Pe_float(cthis->alarm.alarm_backlash, 1, (char *)s);
			break;
		default:
			
			phn_sys.save_chg_flga &= ~CHG_MODCHN_CONF(cthis->chni.chn_NO);
			break;
		
		
	}
	return RET_OK;
}
static int MdlChn_set_by_string(Model *self, IN int aux, void *arg)
{
//	Model_chn		*cthis = SUB_PTR( self, Model, Model_chn);
//	char		*p = (char *)arg;
//	switch(aux) {
//		case AUX_DATA:
//		
//			return p;
//		case AUX_UNIT:
//			if( arg) {
//				p = (char *)arg;		
//			} else {
//				p = cthis->unit_buf;
//			}
//			Mdl_unit_to_string( cthis->chni.unit, p, 8);
//			return p;
//		case AUX_ALARM:
//			if( arg) {
//				p = (char *)arg;		
//			} else {
//				p = cthis->alarm_buf;
//			}
//		
//			memset(p, 0, 8);
//			if(cthis->chni.value > hh) {
//				strcat(p, "HH ");
//			}
//			
//			if(cthis->chni.value > hi) {
//				strcat(p, "Hi ");
//			}
//			
//			if(cthis->chni.value < li) {
//				strcat(p, "Li ");
//			}
//			
//			if(cthis->chni.value < ll) {
//				strcat(p, "LL ");
//			}
//			
//			//把空间填满，来达到清除掉上一次的显示的目的
//			if( strlen( p) < 4) {
//				strcat(p, "   ");
//			}
//			return p;
////		case AUX_PERCENTAGE:
////			if( arg) {
////				p = (char *)arg;		
////			} else {
////				p = cthis->str_buf;
////			}
////			if(cthis->range == 1000)
////				sprintf( p, "%d.%d", cthis->i_rand/10, cthis->i_rand%10);
////			else
////				sprintf( p, "%d", cthis->i_rand);
////			return p;
//			
//		case AUX_SIGNALTYPE:
//			Pe_singnaltype((e_signal_t)cthis->chni.signal_type, (char *)arg);
//			break;
//		case chnaux_lower_limit:
//			Pe_float(cthis->chni.lower_limit, 1, (char *)arg);
//			break;
//		case chnaux_upper_limit:
//			Pe_float(cthis->chni.upper_limit, 1, (char *)arg);
//			break;
//		case chnauxsmall_signal:
//			Pe_float(cthis->chni.small_signal, 1, (char *)arg);
//			strcat((char *)arg, " %");
//			break;
//		case chnaux_k:
//			Pe_float(cthis->chni.k, 2, (char *)arg);
//			break;
//		case chnaux_b:
//			Pe_float(cthis->chni.k, 1, (char *)arg);
//			break;	
//		
//		default:
//			break;
//			
//		
//	}
//	

	return 0;
	
}

static int  MdlChn_to_percentage( Model *self, void *arg)
{
	Model_chn		*cthis = SUB_PTR( self, Model, Model_chn);
	uint8_t			*p = (uint8_t *)arg;
	
	*p = cthis->chni.value;
	
	return RET_OK;
}




static void Pe_singnaltype(e_signal_t sgt, char *str)
{
	switch(sgt)
	{
		case AI_0_5_V:
			sprintf(str, "0~5V");
			break;
		case AI_0_10_mA:
			sprintf(str, "0~10mA");
			break;		
		case AI_1_5_V:
			sprintf(str, "1~5V");
			break;
		case AI_4_20_mA:
			sprintf(str, "4~20mA");
			break;	
		case AI_0_20_mV:
			sprintf(str, "0~20mV");
			break;
		case AI_0_100_mV:
			sprintf(str, "0~100mV");
			break;	
		case AI_Pt100:
			sprintf(str, "Pt100");
			break;
		case AI_Cu50:
			sprintf(str, "Cu50");
			break;
		case AI_B:
			sprintf(str, "B");
			break;		
		case AI_E:
			sprintf(str, "E");
			break;
		case AI_J:
			sprintf(str, "J");
			break;		
		case AI_K:
			sprintf(str, "K");
			break;
		case AI_S:
			sprintf(str, "S");
			break;	
		case AI_T:
			sprintf(str, "T");
			break;
		case AI_0_400_ohm:
			sprintf(str, "0-400O");
			break;		
		case PI_0_30_kHz:
			sprintf(str, "PI");
			break;
		case DI_8_30_V:
			sprintf(str, "DI 8~30V");
			break;		
		case DI_0_5_V:
			sprintf(str, "DI 0~5V");
			break;	
		case AO_4_20_mA:
			sprintf(str, "AO");
			break;			
	}	
}

static void Pe_touch_spot(int spot, char *str)
{
	
	if(spot < 12 && spot > 0)
		sprintf(str, "%d", spot);
	else
		sprintf(str, "无");
}

static void MdlChn_Save_2_conf(mdl_chn_save_t *p_mcs, chn_info_t *p_cnf, uint8_t direct)
{
	
	if(direct == 0)
	{
		p_cnf->b = p_mcs->b;
		p_cnf->decimal = p_mcs->decimal;
		p_cnf->filter_time_s = p_mcs->filter_time_s;
		p_cnf->k = p_mcs->k;
		p_cnf->lower_limit = p_mcs->lower_limit;
		p_cnf->MB = p_mcs->MB;
		p_cnf->signal_type = p_mcs->signal_type;
		p_cnf->small_signal = p_mcs->small_signal;
		p_cnf->tag_NO = p_mcs->tag_NO;
		p_cnf->unit = p_mcs->unit;
		p_cnf->upper_limit = p_mcs->upper_limit;
		
		
		
		
	}
	else
	{
		p_mcs->b = p_cnf->b;
		p_mcs->decimal = p_cnf->decimal;
		p_mcs->filter_time_s = p_cnf->filter_time_s;
		p_mcs->k = p_cnf->k;
		p_mcs->lower_limit = p_cnf->lower_limit;
		p_mcs->MB = p_cnf->MB;
		p_mcs->signal_type = p_cnf->signal_type;
		p_mcs->small_signal = p_cnf->small_signal;
		p_mcs->tag_NO = p_cnf->tag_NO;
		p_mcs->unit = p_cnf->unit;
		p_mcs->upper_limit = p_cnf->upper_limit;
		
	}
	
}

static void MdlChn_Save_2_alarm(mdl_chn_save_t *p_mcs, chn_alarm_t *p_alr, uint8_t direct)
{
	if(direct == 0)
	{
		p_alr->alarm_backlash = p_mcs->alarm_backlash;
		p_alr->alarm_hh = p_mcs->alarm_hh;
		p_alr->alarm_hi = p_mcs->alarm_hi;
		p_alr->alarm_ll = p_mcs->alarm_ll;
		p_alr->alarm_lo = p_mcs->alarm_lo;
		p_alr->touch_spot_hh = p_mcs->touch_spot_hh;
		p_alr->touch_spot_hi = p_mcs->touch_spot_hi;
		p_alr->touch_spot_ll = p_mcs->touch_spot_ll;
		p_alr->touch_spot_lo = p_mcs->touch_spot_lo;
		
		
		
		
		
	}
	else
	{
		p_mcs->alarm_backlash = p_alr->alarm_backlash;
		p_mcs->alarm_hh = p_alr->alarm_hh;
		p_mcs->alarm_hi = p_alr->alarm_hi;
		p_mcs->alarm_ll = p_alr->alarm_ll;
		p_mcs->alarm_lo = p_alr->alarm_lo;
		p_mcs->touch_spot_hh = p_alr->touch_spot_hh;
		p_mcs->touch_spot_hi = p_alr->touch_spot_hi;
		p_mcs->touch_spot_ll = p_alr->touch_spot_ll;
		p_mcs->touch_spot_lo = p_alr->touch_spot_lo;
		
	}
	
	
}

//static int Str_float_to_int(char *str, int prec)
//{
//	
//	return 0;
//}

