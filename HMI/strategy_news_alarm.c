#include <stdint.h>
#include "HMI_striped_background.h"
#include "utils/Storage.h"

//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------

static int NLM_Entry(int row, int col, void *pp_text);
static int NLM_Init(void *arg);
static void NLM_Build_component(void *arg);
static int NLM_Key_UP(void *arg);
static int NLM_Key_DN(void *arg);
static int NLM_Key_LT(void *arg);
static int NLM_Key_RT(void *arg);
static int NLM_Key_ET(void *arg);
static int NLM_Get_focus_data(void *pp_data, strategy_focus_t *p_in_syf);
static int NLM_Commit(void *arg);
static void NLM_Exit(void);

strategy_t	g_news_alarm = {
	NLM_Entry,
	NLM_Init,
	NLM_Build_component,
	NLM_Key_UP,
	NLM_Key_DN,
	NLM_Key_LT,
	NLM_Key_RT,
	NLM_Key_ET,
	NLM_Get_focus_data,	
	NLM_Commit,
	NLM_Exit,
};

#define STRT_SELF			g_news_alarm

//static int Cns_init(void *arg);
//static int Cns_get_focusdata(void *pp_data, strategy_focus_t *p_in_syf);
//strategy_t	g_chn_strategy = {
//	ChnStrategy_entry,
//	Cns_init,
//	Cns_key_up,
//	Cns_key_dn,
//	Cns_key_lt,
//	Cns_key_rt,
//	Cns_key_er,
//	Cns_get_focusdata,
//};
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
 static char *const arr_NLM_col_0[5] = {"NO", "通道", "类型", "报警开始", "报警结束"};
	
//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static void	NLM_Btn_hdl(void *self, uint8_t	btn_id);
//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//

static int NLM_Entry(int row, int col, void *pp_text)
{
	char 						**pp = (char **)pp_text;
	Storage					*stg = Get_storage();
	rcd_alm_pwr_t		stg_alm;
	struct 		tm 		t;
	int							r = 0;
	if(col >4)
		return 0;
	
	r = row % STRIPE_MAX_ROWS;		//条纹界面上的行数是11
	
	if(row == 0)
	{
		
		*pp = arr_NLM_col_0[col];
		return strlen(arr_NLM_col_0[col]);
	}
	
	STG_Set_file_position(STG_CHN_ALARM(g_setting_chn), STG_DRC_READ, row * sizeof(rcd_alm_pwr_t));
	if(stg->rd_stored_data(stg, STG_CHN_ALARM(g_setting_chn), \
			&stg_alm, sizeof(rcd_alm_pwr_t)) != sizeof(rcd_alm_pwr_t))
		{	
			//或者已经读完了
			return 0;
		}
		
	if(stg_alm.flag == 0xff)
		return 0;		//记录结尾
		
	switch(col)
	{
		case 0:
			sprintf(arr_p_vram[r], "%d", row);
			break;
		case 1:
			sprintf(arr_p_vram[r], "CHN%d", g_setting_chn);
			break;
		case 2:
			switch(stg_alm.alm_pwr_type)
			{
				case ALM_CODE_HH:
					sprintf(arr_p_vram[r], "HH");
					break;
				case ALM_CODE_HI:
					sprintf(arr_p_vram[r], "HI");
					break;
				case ALM_CODE_LO:
					sprintf(arr_p_vram[r], "LO");
					break;
				case ALM_CODE_LL:
					sprintf(arr_p_vram[r], "LL");
					break;
				default:
					//不应该出现在这里
					sprintf(arr_p_vram[r], "  ");
					break;
			}
			break;
		case 3:
			Sec_2_tm(stg_alm.happen_time_s, &t);
			sprintf(arr_p_vram[r], "%2d/%02d/%02d %02d:%02d:%02d", t.tm_year,t.tm_mon, t.tm_mday, \
					t.tm_hour, t.tm_min, t.tm_sec);
			break;
		case 4:
			Sec_2_tm(stg_alm.disapper_time_s, &t);
			sprintf(arr_p_vram[r], "%2d/%02d/%02d %02d:%02d:%02d", t.tm_year,t.tm_mon, t.tm_mday, \
					t.tm_hour, t.tm_min, t.tm_sec);
			break;
		default:
			return 0;
		
	}
	return strlen(arr_p_vram[r]);

}

static int NLM_Init(void *arg)
{
	int i = 0;
	
	
	VRAM_init();
	for(i = 0; i < 11; i++) {
		arr_p_vram[i] = VRAM_alloc(48);
		memset(arr_p_vram[i], 0, 48);
	}
	
	g_setting_chn = 0;
	
	return RET_OK;
}

static void NLM_Build_component(void *arg)
{
	Button			*p_btn = BTN_Get_Sington();

	p_btn->build_each_btn(0, BTN_TYPE_MENU, Setting_btn_hdl, arg);
	p_btn->build_each_btn(3, BTN_TYPE_ERASE, NLM_Btn_hdl, arg);
		
}
static void NLM_Exit(void)
{
	
}
static int NLM_Commit(void *arg)
{
	return RET_OK;
	
}

static int NLM_Get_focus_data(void *pp_data, strategy_focus_t *p_in_syf)
{

	

	return 0;
	
}

static int NLM_Key_UP(void *arg)
{
	
	g_setting_chn ++;
	if(g_setting_chn == NUM_CHANNEL)
		g_setting_chn = 0;
	
	return -1;
}
static int NLM_Key_DN(void *arg)
{
	if(g_setting_chn)
		g_setting_chn --;
	else
		g_setting_chn = NUM_CHANNEL - 1;
	
	return -1;
}
static int NLM_Key_LT(void *arg)
{
	
	return -1;
}

static int NLM_Key_RT(void *arg)
{
	
	return -1;
}
static int NLM_Key_ET(void *arg)
{
	return -1;
	
}

static void	NLM_Btn_hdl(void *self, uint8_t	btn_id)
{
	if(btn_id == ICO_ID_ERASETOOL)
		STG_Erase_file(STG_CHN_ALARM(g_setting_chn));
	
}




