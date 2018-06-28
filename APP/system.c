//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//
#include "HMI/HMIFactory.h"

#include "system.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdhDef.h"
#include "utils/rtc_pcf8563.h"
#include "utils/hw_w25q.h"
#include "utils/FM25.h"
#include "utils/log.h"

#include "fs/easy_fs.h"
#include "utils/Storage.h"

#include "Modbus_app.h"
#include "channel_accumulated.h"
#include "cfg_test.h"
//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
#define 	PHN_MAJOR_VER				3
#define 	PHN_MINOR_VER				24

#define 	PHN_DEF_SUPER_PSD_1			0x01			
#define 	PHN_DEF_SUPER_PSD_2			0x01
#define 	PHN_DEF_SUPER_PSD_3			0x01



//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------

static UtlRtc *sys_rtc;	
//Ϊ���ܹ����յĶ���һЩ��̬���������Զ�������һ��
//ֻ�з�4�ֽڶ���ģ���Ҫ�������ﶨ��
char 				*arr_p_vram[18];
uint16_t			time_smp;
//char				g_setting_chn = 0;
char				flush_flag = 0;
//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------
system_t		phn_sys;
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


//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//Ҫ�ڴ洢���ܳ�ʼ��֮�����
void System_power_on(void)
{
	Storage			*stg = Get_storage();
	rcd_alm_pwr_t			stg_pwr = {0};
	int						num_pwr = 0;
	
	
	
	
	
//	STG_Set_file_position(STG_LOSE_PWR, STG_DRC_READ, 0);
//	while(stg_pwr.flag != 0xff)
//	{
//		if(stg->rd_stored_data(stg, STG_LOSE_PWR, \
//			&stg_pwr, sizeof(rcd_alm_pwr_t)) != sizeof(rcd_alm_pwr_t))
//			{
//				
//				//�����Ѿ�������
//				break;
//				
//				
//			}
//			if(stg_pwr.flag != 0xff)
//				num_pwr ++;
//		
//	}
//	
//	if(num_pwr == STG_MAX_NUM_LST_PWR)
//		num_pwr = 0;	
//	phn_sys.pwr_rcd_index = num_pwr;

	phn_sys.pwr_rcd_index = STG_Get_alm_pwr_num(STG_LOSE_PWR);
	if(phn_sys.pwr_rcd_index > STG_MAX_NUM_LST_PWR)
		phn_sys.pwr_rcd_index = 0;

	//��¼�ϵ�ʱ��
	stg_pwr.flag = 1;
	stg_pwr.happen_time_s = SYS_time_sec();
	stg_pwr.disapper_time_s = 0xffffffff;
	STG_Set_file_position(STG_LOSE_PWR, STG_DRC_WRITE, phn_sys.pwr_rcd_index * sizeof(rcd_alm_pwr_t));
	
	stg->wr_stored_data(stg, STG_LOSE_PWR, &stg_pwr, sizeof(rcd_alm_pwr_t));
	
	//�ڼ�¼����������
	num_pwr = phn_sys.pwr_rcd_index + 1;
	num_pwr %= STG_MAX_NUM_LST_PWR;
	STG_Set_alm_pwr_num(STG_LOSE_PWR, num_pwr);
	
}

void System_power_off(void)
{
	short 				retry = 5;
	short				i = 0;
	uint32_t		dsp_time = 0;
	Storage			*stg = Get_storage();
	
	phn_sys.sys_flag |= SYSFLAG_URGENCY;
	//������Ϣ��Чʱ���Ͳ�Ҫ�洢����ʱ����
	//����ִ���˲���������Ϣ������ʱ�򣬻�����������
	if(phn_sys.pwr_rcd_index != 0xff)
	{
	
		
	
		dsp_time = SYS_time_sec();
		STG_Set_file_position(STG_LOSE_PWR, STG_DRC_WRITE, \
		phn_sys.pwr_rcd_index * sizeof(rcd_alm_pwr_t) +(int)(&((rcd_alm_pwr_t *)0)->disapper_time_s));
		
		while(stg->wr_stored_data(stg, STG_LOSE_PWR, &dsp_time, sizeof(uint32_t)) != sizeof(uint32_t))
		{
			if(retry)
				retry --;
			else
				break;
			
		}
	}
	for(i = 0; i < NUM_CHANNEL; i++)
		MCH_Cancle_all_alarm(i);
//	stg->wr_stored_data(stg, STG_SYS_CONF, &phn_sys.sys_conf, sizeof(phn_sys.sys_conf));
	
	phn_sys.fs.fs_shutdown();
	phn_sys.sys_flag &= ~SYSFLAG_URGENCY;

}

void SYS_Reset(void)
{
	int chn_num;
	EFS_Reset();
	System_default();
	for(chn_num = 0; chn_num < NUM_CHANNEL; chn_num ++)
	{
		MdlChn_default_conf(chn_num);
		MdlChn_default_alarm(chn_num);
		MdlChn_Commit_conf(chn_num);
		
	}
	STG_Reset();
	CNA_default();
	
}

void System_default(void)
{
	system_conf_t 	*p_sc = &phn_sys.sys_conf;
	Storage			*stg = Get_storage();
	uint8_t		password[3];
	uint8_t		super_psd[3];
	
	//���벻Ҫ���⸴λ
	
	Clone_psd(p_sc->super_psd, super_psd);
	Clone_psd(p_sc->password, password);
	memset(p_sc, 0, sizeof(system_conf_t));
	Clone_psd(super_psd, p_sc->super_psd);
	Clone_psd(password, p_sc->password);
	
	
	p_sc->sys_flag = 0;
	p_sc->num_chn = NUM_CHANNEL;
	p_sc->cold_end_way = 0;
	p_sc->id = 1;
	p_sc->record_gap_s = 1;
	
	p_sc->baud_idx = 2;
	p_sc->baud_rate = arr_baud[2];
	p_sc->show_chn_status = 0;
	p_sc->storage_alarm = STG_DEFAULT_ALARM;
	
	
	stg->wr_stored_data(stg, STG_SYS_CONF, &phn_sys.sys_conf, sizeof(phn_sys.sys_conf));
	
	if((p_sc->super_psd[0] == 0xff) && (p_sc->super_psd[1] == 0xff) && (p_sc->super_psd[2] == 0xff))
	{
		p_sc->super_psd[0] = PHN_DEF_SUPER_PSD_1;
		p_sc->super_psd[1] = PHN_DEF_SUPER_PSD_2;
		p_sc->super_psd[2] = PHN_DEF_SUPER_PSD_3;
	}
	
	if((p_sc->password[0] == 0xff) && (p_sc->password[1] == 0xff) && (p_sc->password[2] == 0xff))
	{
		p_sc->password[0] = 0;
		p_sc->password[1] = 0;
		p_sc->password[2] = 0;
	}
	

}

void System_init(void)
{
//	struct  tm stm;
	Model 		*m;
	Storage					*stg = Get_storage();
	char			chn_name[7];
	char			i;
//	int				retry = 5;

	phn_sys.major_ver = PHN_MAJOR_VER;
	phn_sys.minor_ver = PHN_MINOR_VER;
	
	
	sys_rtc = ( UtlRtc *)Pcf8563_new();
	if(sys_rtc == NULL) while(1);

	sys_rtc->init(sys_rtc, NULL);
	sys_rtc->get(sys_rtc, &phn_sys.sys_time);
	
	//md_timeҪϵͳʱ���ʼ��֮���ʼ��
	m = Create_model("time");
	m->init(m, NULL);
	
	
	
	w25q_init();
	FM25_init();
	
#if UNT_ON	== 0
	EFS_init(NUM_FSH, 0);
#else
	EFS_init(NUM_FSH, 0);
#endif
	
#if UNT_ON	== 0
	stg->init(stg);
	
	
	
	/*  �����е��ļ����չ̶���˳���һ�飬����֤ÿ���Ǳ��ڵ�һ���ϵ�ʱ���ļ��Ĵ洢λ����һ�µ�*/
	//����ֻ����FM25�ϵ��ļ�
	//ͨ���������ļ��������и����Ⱥ�˳������
	stg->open_file(STG_SYS_CONF, STG_DEF_FILE_SIZE);
	
	//������Ϣ�Լ�����ͨ���ı������ۻ���Ϣ������ͬһ���ļ�����ģ���˴�һ�ξ���
	stg->open_file(STG_CHN_ALARM(0), STG_DEF_FILE_SIZE);			

	stg->open_file(STG_LOG, STG_DEF_FILE_SIZE);
	
	System_power_on();
	
	LOG_Init();
	stg->rd_stored_data(stg, STG_SYS_CONF, &phn_sys.sys_conf, sizeof(phn_sys.sys_conf));
	if((phn_sys.sys_conf.num_chn > NUM_CHANNEL) || (phn_sys.sys_conf.num_chn == 0))
		System_default();
	
	
	CNA_Init();
	for(i = 0; i < NUM_CHANNEL; i++)
	{

		sprintf(chn_name,"chn_%d", i);
		m = Create_model(chn_name);
		m->init(m, &i);
		
	}
	
	MdlChn_Read_code_end_temperature();
//	System_power_off();
#endif	
	
}

//todo:��ζ���Ĵ���
extern void Ctime_Allco_time(uint16_t  all_time, uint8_t need);
int SYS_Commit(void)
{
	Storage					*stg = Get_storage();
	
	stg->wr_stored_data(stg, STG_SYS_CONF, &phn_sys.sys_conf, sizeof(phn_sys.sys_conf));
	Ctime_Allco_time(phn_sys.sys_conf.record_gap_s, NUM_CHANNEL);
	
	//Modbus�Ĵ��ڲ���Ҳ���ܱ��޸��ˣ�����Ҫ���³�ʼ��MODBUS�Ĵ���
	MBA_Init_uart(phn_sys.sys_conf.baud_rate);
	return RET_OK;
}
void System_time(struct  tm *stime)
{
	
	sys_rtc->get(sys_rtc, stime);
}


int  System_set_time(struct  tm *stime)
{
	int ret;
	ret = sys_rtc->set(sys_rtc, stime);
	
	if(ret == RET_OK)
		sys_rtc->get(sys_rtc, stime);
	
	return ret;
}



uint32_t  SYS_time_sec(void)
{
	
	return Time_2_u32(&phn_sys.sys_time);
//	Model 		*m;
//	uint32_t	sec = 0;
//	m = Create_model("time");
//	m->getMdlData(m, TIME_U32, &sec);
	
//	return sec;
	
	
//	struct tm  t;
//	sys_rtc->get(sys_rtc, &t);
//	
//	return Time_2_u32(&t);
}










//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{





