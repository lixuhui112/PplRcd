#include <stdint.h>
#include "HMI_striped_background.h"
#include "system.h"
#include "USB/Usb.h"
#include "os/os_depend.h"
#include "sys_cmd.h"
#include "utils/Storage.h"
#include "utils/log.h"
#include "utils/time_func.h"

//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
//#define DBP_ID		0x01

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------

static int Data_bacnup_Strategy_entry(int row, int col, void *pp_text);
static int DBP_init(void *arg);
static void DBP_build_component(void *arg);
static int DBP_key_up(void *arg);
static int DBP_key_dn(void *arg);
static int DBP_key_lt(void *arg);
static int DBP_key_rt(void *arg);
static int DBP_key_er(void *arg);
static int DBP_get_focusdata(void *pp_data,  strategy_focus_t *p_in_syf);
static int DBP_commit(void *arg);
static void DBP_Exit(void);
strategy_t	g_DBP_strategy = {
	Data_bacnup_Strategy_entry,
	DBP_init,
	DBP_build_component,
	DBP_key_up,
	DBP_key_dn,
	DBP_key_lt,
	DBP_key_rt,
	DBP_key_er,
	DBP_get_focusdata,
	DBP_commit,
	DBP_Exit,
};


//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
#define STG_SELF  g_DBP_strategy

enum {
	row_usb_info,
	row_file_type,
	row_first_chn,
	row_last_chn,
	row_start_time,
	row_end_time,
	row_file_name,
	row_max,
//	DBP_row_tips,
//	DBP_row_temp,
}DBP_rows;


#define BDP_USB_BUF_SIZE			USB_MAX_WRITE_BYTE



//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------



typedef struct {
	
	uint8_t		arr_DBP_fds[4];
	uint8_t		DBP_copy;
	
	char		copy_file_type;	//0 ���� 1 ���� 2 ���� 3 log
	uint8_t		copy_first_chn;
	uint8_t		copy_last_buf;
	
	char		tip_buf[48];
	
	//rcd_buf����̫�󣬷����ȡ��������usb_buf����Ų���
	
	uint8_t		rcd_buf[128];	
	char		usb_buf[BDP_USB_BUF_SIZE];
}dbp_run_t;


#define DBP_NUM_RAM					(row_max)
#define STG_RUN_VRAM_NUM			row_max		

#define DBP_FIRST_CHN		p_run->copy_first_chn
#define DBP_LAST_CHN		p_run->copy_last_buf

#define STG_P_RUN		(dbp_run_t *)arr_p_vram[STG_RUN_VRAM_NUM];
#define INIT_RUN_RAM do { \
	arr_p_vram[STG_RUN_VRAM_NUM] = HMI_Ram_alloc(sizeof(dbp_run_t)); \
	memset(arr_p_vram[STG_RUN_VRAM_NUM], 0, sizeof(dbp_run_t)); \
}while(0)

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
 static char *const arr_p_DBU_entry[8] = {"�豸��ǰ״̬��", "�������ͣ�","��ʼͨ����", "��βͨ����", "��ʼʱ�䣺", "��ֹʱ�䣺",\
	 "�ļ�����", "���ݽ���"
 };
 

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static int	DBP_Usb_event(int type);
static int DBP_update_content(int op, int weight);
static void	DBP_Btn_hdl(void *self, uint8_t	btn_id); 
static void DBP_Print_file_type(char *s, char ft);
static void DBP_Default_file_name(char *s, char ft);
 
static int	DBP_filename_commit(void *self, void *data, int len);
 
 static void DBP_Focus_file_name(void);	//�ļ�����ǰ׺'/' �� ��׺'.CSV'������ѡ��
 
static void DBP_Copy(void);
static uint32_t DBP_Copy_chn_data(int fd);
static uint32_t DBP_Copy_chn_alarm(int fd);
static uint32_t DBP_Copy_lost_power(int fd);
static uint32_t DBP_Copy_log(int fd);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

 

 
 

 
//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//

static int Data_bacnup_Strategy_entry(int row, int col, void *pp_text)
{
	dbp_run_t *p_run ;
	char **pp = (char **)pp_text;
	Model	*model;

	
	p_run = STG_P_RUN;
	
	
	if(col == 0) {
		
		if(row > row_max)
			return 0;
		*pp = arr_p_DBU_entry[row];
		return strlen(arr_p_DBU_entry[row]);
	} 
	else if(col == 1)
	{
		switch(row) 
		{
			case row_usb_info:
				if(phn_sys.usb_device)
					sprintf(arr_p_vram[row], "�豸������");
				else
					sprintf(arr_p_vram[row], "�豸δ����");
				break;
			case row_file_type:
				DBP_Print_file_type(arr_p_vram[row], p_run->copy_file_type);
				break;
			case row_first_chn:		//
				sprintf(arr_p_vram[row], "%d", DBP_FIRST_CHN);
				break;
			case row_last_chn:		//
				sprintf(arr_p_vram[row], "%d", DBP_LAST_CHN);
				break;
			case row_start_time:	
			case row_end_time:		//��λ
				if(arr_p_vram[row][0] != '\0')
					break;		//��������Ǵ����ô��ڷ��أ����ԾͲ�Ҫ�ٸ�ֵԭʼֵ�ˣ�����Ĳ���Ҳ��һ��
				model = Create_model("time");
				model->to_string(model, 1, arr_p_vram[row]);
				break;
			case row_file_name:		
				if(arr_p_vram[row][0] == '\0')
				{
//					sprintf(arr_p_vram[row], "/CHN_%d.CSV", p_run->cur_chn);	//��72Ҫ���ļ��������д
					DBP_Default_file_name(arr_p_vram[row], p_run->copy_file_type);
				}
				else {
					
					if( strstr(arr_p_vram[row], ".CSV") == NULL)
					{
						
						strcat(arr_p_vram[row], ".CSV");
					}
					
				}
				break;
			default:
				goto exit;
		}
		*pp = arr_p_vram[row];
		return strlen(arr_p_vram[row]);
		
	}
	exit:
	return 0;
}

static int DBP_init(void *arg)
{
	int			i;
	dbp_run_t 	*p_run;
	
	kbr_cmt = DBP_filename_commit;

	memset(&g_DBP_strategy.sf, 0, sizeof(g_DBP_strategy.sf));
	g_DBP_strategy.sf.f_col = 1;
	g_DBP_strategy.sf.f_row = 1;
	g_DBP_strategy.sf.start_byte = 0;
	g_DBP_strategy.sf.num_byte = 1;
	HMI_Ram_init();
	
	for(i = 0; i < DBP_NUM_RAM; i++) {
		
		arr_p_vram[i] = HMI_Ram_alloc(48);
		memset(arr_p_vram[i], 0, 48);
	}
	
	INIT_RUN_RAM;
	p_run = STG_P_RUN;
	
	STG_SELF.total_col = 2;
	STG_SELF.total_row = row_max;
//	BDP_USB_BUF_SIZE = HMI_Ram_free_bytes();
	
	
	DBP_FIRST_CHN = 0;
	DBP_LAST_CHN = phn_sys.sys_conf.num_chn - 1;
	
	p_run->arr_DBP_fds[0] = USB_Rgt_event_hdl(DBP_Usb_event);
//	phn_sys.key_weight = 1;
	return RET_OK;
}

static void DBP_build_component(void *arg)
{
	Button			*p_btn = BTN_Get_Sington();
	Progress_bar	*p_bar = PGB_Get_Sington();
	bar_object_t	bob = {{1, 0, 270, STRIPE_SIZE_Y, 0, PGB_TWD_CROSS, FONT_16, PGB_TIP_RIGHT}, \
		{COLOUR_GREN, COLOUR_GRAY, COLOUR_BLUE, COLOUR_YELLOW}};
	
	dbp_run_t 		*p_run = STG_P_RUN;
	
		
		
		
	bob.bar_frm.bar_y0 = Stripe_vy(row_max + 1);
	p_btn->build_each_btn(0, BTN_TYPE_MENU, Setting_btn_hdl, arg);
	p_btn->build_each_btn(1, BTN_TYPE_COPY, DBP_Btn_hdl, arg);
	p_btn->build_each_btn(2, BTN_TYPE_STOP, DBP_Btn_hdl, arg);
		
	p_run->arr_DBP_fds[1] = p_bar->build_bar(&bob);
//	p_bar->update_bar(g_DBP_strategy.sty_some_fd, 50);
	
}
static void DBP_Exit(void)
{
	Progress_bar	*p_bar = PGB_Get_Sington();
	dbp_run_t 		*p_run = STG_P_RUN;
	USB_Del_event_hdl(p_run->arr_DBP_fds[0]);
	
	//������ڿ�����ʱ�򣬰�ESC�˳�
	//Ӧ���ÿ�������ֹͣ����
	if(p_run->DBP_copy)
		p_run->DBP_copy = 0;
	p_bar->delete_bar(p_run->arr_DBP_fds[1]);
	kbr_cmt = NULL;
}

static void DBP_Default_file_name(char *s, char ft)
{
	switch(ft)
	{
		case 0:
			sprintf(s,  "/DATA.CSV  ");
			break;
		case 1:
			sprintf(s, "/ALARM.CSV  ");
			break;
		case 2:
			sprintf(s, "/POWER.CSV  ");
			break;
		case 3:
			sprintf(s, "/SDHLOG.CSV  ");
			break;
		default:
			sprintf(s, "         ");
			break;
		
	}
	
}
static void DBP_Print_file_type(char *s, char ft)
{
	switch(ft)
	{
		case 0:
			sprintf(s, "��ʷ����");
			break;
		case 1:
			sprintf(s, "������Ϣ");
			break;
		case 2:
			sprintf(s, "������Ϣ");
			break;
		default:
			sprintf(s, "********");
			break;
		
	}
	
	
}
static int DBP_key_up(void *arg)
{
//	strategy_keyval_t	kt = {SY_KEYTYPE_HIT};
	int 			ret = RET_OK;
	
//	if(arg) {
//		kt.key_type = ((strategy_keyval_t *)arg)->key_type;
//		
//	}
//	if(kt.key_type == SY_KEYTYPE_LONGPUSH) {
//		phn_sys.key_weight += 10;
//		
//	} else {
//		phn_sys.key_weight = 1;
//	}

	


	ret = DBP_update_content(OP_ADD, phn_sys.key_weight);
	return ret;
}
static int DBP_key_dn(void *arg)
{
	
//	strategy_keyval_t	kt = {SY_KEYTYPE_HIT};
//	strategy_focus_t *p_syf = &g_sys_strategy.sf;
	int 			ret = RET_OK;
	
//	if(arg) {
//		kt.key_type = ((strategy_keyval_t *)arg)->key_type;
//		
//	}
//	if(kt.key_type == SY_KEYTYPE_LONGPUSH) {
//		phn_sys.key_weight += 10;
//		
//	} else {
//		phn_sys.key_weight = 1;
//	}
	
	ret = DBP_update_content(OP_SUB, phn_sys.key_weight);
	return ret;
}

static void DBP_Focus_file_name(void)
{
	strategy_focus_t *p_syf = &g_DBP_strategy.sf;
			
	if(p_syf->f_row == row_file_name)
	{
		p_syf->num_byte = strcspn(arr_p_vram[p_syf->f_row], "."); 
		p_syf->start_byte = 1;
		p_syf->num_byte -= 1;	// ǰ׺'/' �� ��׺�������޸�
	}
}

static int DBP_key_lt(void *arg)
{
	
	strategy_focus_t *p_syf = &g_DBP_strategy.sf;
	int ret = RET_OK;
//	if(p_run->copy_file_type)
//	{
//		//����ʷ���ݾ�ֻ��Ҫѡ���ļ������ļ�����
//		if(p_syf->f_row == row_file_name)
//			p_syf->f_row = row_file_type;
//		else
//		{
//			p_syf->f_row = row_file_name;
//			ret = -1;
//		}
//		
//	}
//	else
	{
		//��һ����ʾ״̬��������ѡ��
		if(p_syf->f_row > 2)
			p_syf->f_row --;
		else {
//			p_syf->f_row = row_max - 1;
			p_syf->f_row = 1;		
			ret = -1;
			
		}
	}
	p_syf->num_byte = strlen(arr_p_vram[p_syf->f_row]);
	p_syf->start_byte = 0;
	DBP_Focus_file_name();
	return ret;
}
static int DBP_key_rt(void *arg)
{
	
	strategy_focus_t *p_syf = &g_DBP_strategy.sf;
	int ret = RET_OK;
	
//	if(p_run->copy_file_type)
//	{
//		//����ʷ���ݾ�ֻ��Ҫѡ���ļ������ļ�����
//		if(p_syf->f_row == row_file_type)
//			p_syf->f_row = row_file_name;
//		else
//		{
//			p_syf->f_row = row_file_type;
//			ret = -1;
//		}
//		
//	}
//	else
	{
	
		if(p_syf->f_row < (row_max - 1))
			p_syf->f_row ++;
		else {
			p_syf->f_row = 1;
			p_syf->f_col = 1;
			ret = -1;
		}
	}
	
	p_syf->num_byte = strlen(arr_p_vram[p_syf->f_row]);
	p_syf->start_byte = 0;
	DBP_Focus_file_name();
	return ret;
}
static int DBP_key_er(void *arg)
{
	return RET_OK;
}



static int DBP_get_focusdata(void *pp_data,  strategy_focus_t *p_in_syf)
{
	
	strategy_focus_t *p_syf = &g_DBP_strategy.sf;
	char		**pp_vram = (char **)pp_data;
	int ret = 0;
	
	if((p_syf->f_row < 1) || (p_syf->f_row > (row_max - 1))) {
		return -1;
	}
	
	if(p_in_syf)
		p_syf = p_in_syf;
	
	
	
	
	p_syf->start_byte = 0;
	p_syf->num_byte = strlen(arr_p_vram[p_syf->f_row]);
	DBP_Focus_file_name();
	
	ret = p_syf->num_byte;
	*pp_vram = arr_p_vram[p_syf->f_row] + p_syf->start_byte;
	return ret;
}
static int DBP_commit(void *arg)
{
	struct  tm	 t;
	strategy_focus_t *p_syf = &g_DBP_strategy.sf;
	
	if((p_syf->f_row == row_start_time) || (p_syf->f_row == row_end_time))
	{
		//��Ҫ���ж�һ�������õ���ֹʱ���Ƿ�Ϸ�
		return TMF_Str_2_tm(arr_p_vram[p_syf->f_row], &t);
		
	}
	//��Ҫ���ж�һ�������õ���ֹʱ���Ƿ�Ϸ�
	return RET_OK;
}


static int	DBP_Usb_event(int type)
{
	strategy_focus_t		pos;

	if((type != et_ready) && (type != et_remove))
		return RET_OK;
	
	pos.f_col = 1;
	pos.f_row = row_usb_info;
	g_DBP_strategy.cmd_hdl(g_DBP_strategy.p_cmd_rcv, sycmd_reflush_position, &pos);
//	if(type == et_ready)
//	{
//		
//		
//	}
//	else if(type == et_remove)
//	{
//		
//		
//	}
	
	return 0;
}


static int	DBP_filename_commit(void *self, void *data, int len)
{
	int i = 0;
	char *p = (char *)data;
	
	//ch372Ҫ���ļ����ֵĳ���С��13���ַ����������ļ������ó���8���ַ�
	if(len > 7)
		len = 7;
	
	memset(arr_p_vram[row_file_name], 0, strlen(arr_p_vram[row_file_name]));
	arr_p_vram[row_file_name][0] = '/';
	for(i = 0; i < len; i ++)
	{
		//��Сдת���ɴ�д
		if(p[i] <= 'z' && p[i] >= 'a')
		{
			p[i] -= 32;
			
		}
		arr_p_vram[row_file_name][i + 1] = p[i];
		
	}
//	if(arr_p_vram[row_file_name][0] != '/')
//		arr_p_vram[row_file_name][0] = '/';
	return RET_OK;
}

static int DBP_update_content(int op, int weight)
{
	strategy_focus_t 	*p_syf = &g_DBP_strategy.sf;
	int					ret = RET_OK;
	strategy_focus_t		pos;
	dbp_run_t *p_run = STG_P_RUN;

	
	
	switch(p_syf->f_row) {
		case row_file_type:
			p_run->copy_file_type = Operate_in_range(p_run->copy_file_type, op, 1, 0, 2);
			DBP_Print_file_type(arr_p_vram[row_file_type], p_run->copy_file_type);
			pos.f_col = 1;
			pos.f_row = row_file_name;
			arr_p_vram[row_file_name][0] = 0;	//��������Ĭ���ļ���
			g_DBP_strategy.cmd_hdl(g_DBP_strategy.p_cmd_rcv, sycmd_reflush_position, &pos);
			break;
		case row_first_chn:
			DBP_FIRST_CHN = Operate_in_range(DBP_FIRST_CHN, op, 1, 0, phn_sys.sys_conf.num_chn - 1);
			sprintf(arr_p_vram[p_syf->f_row], "%d", DBP_FIRST_CHN);
//			pos.f_col = 1;
//			pos.f_row = row_file_name;
//			arr_p_vram[row_file_name][0] = 0;	//��������Ĭ���ļ���
//			g_DBP_strategy.cmd_hdl(g_DBP_strategy.p_cmd_rcv, sycmd_reflush_position, &pos);
			break;
		case row_last_chn:
			DBP_LAST_CHN = Operate_in_range(DBP_LAST_CHN, op, 1, DBP_FIRST_CHN, phn_sys.sys_conf.num_chn - 1);
			sprintf(arr_p_vram[p_syf->f_row], "%d", DBP_LAST_CHN);
//			pos.f_col = 1;
//			pos.f_row = row_file_name;
//			arr_p_vram[row_file_name][0] = 0;	//��������Ĭ���ļ���
//			g_DBP_strategy.cmd_hdl(g_DBP_strategy.p_cmd_rcv, sycmd_reflush_position, &pos);
			break;

		case row_start_time:		
//			g_DBP_strategy.cmd_hdl(g_sys_strategy.p_cmd_rcv, sycmd_win_time, arr_p_vram[p_syf->f_row]);
//			ret = 1;
//			break;
		case row_end_time:
			g_DBP_strategy.cmd_hdl(g_DBP_strategy.p_cmd_rcv, sycmd_win_time, arr_p_vram[p_syf->f_row]);
			ret = 1;
			break;
		case row_file_name:
			
			g_DBP_strategy.cmd_hdl(g_DBP_strategy.p_cmd_rcv, sycmd_keyboard, arr_p_vram[p_syf->f_row] + 1);	//��һ��'/'�����б༭
			ret = 1;
			break;
	default:
		break;
	
	
	}
	return ret;
}




static void DBP_Copy(void)
{
//	uint32_t			start_sec;
//	uint32_t			old_start = Str_time_2_u32(arr_p_vram[row_start_time]);
//	uint32_t			end_sec = Str_time_2_u32(arr_p_vram[row_end_time]);
//	uint32_t			total = end_sec - old_start + 1;
//	uint32_t			done = 0;
//	uint32_t			rd_sec = 0;
	Progress_bar		*p_bar = PGB_Get_Sington();
	char				*copy_buf;
//	int					rd_len = 0;
	int					usb_fd = 0;
	uint32_t			dbp_count_bytes = 0;
//	uint8_t				last_prc = 0, prc = 0;
//	uint8_t				copy_num_chn = DBP_LAST_CHN - DBP_FIRST_CHN  + 1;
//	uint8_t				copy_chn = DBP_FIRST_CHN;
//	uint8_t				done_chn = 0;
//	usb_fd = USB_Open_file(arr_p_vram[4], USB_FM_WRITE | USB_FM_COVER);
	dbp_run_t 			*p_run = STG_P_RUN;


	if(strcmp(arr_p_vram[row_file_name], "/SDHLOG.CSV") == 0)
		p_run->copy_file_type = 3;
	copy_buf = p_run->usb_buf;
	//
	while(phn_sys.usb_device == 0)
	{
			delay_ms(50);
			if(p_run->DBP_copy == 0)
				goto exit;
		
	}
			

	
	//���ļ�����д���һ�б���
	
	while(usb_fd == 0)
	{
		usb_fd = USB_Create_file(arr_p_vram[row_file_name], USB_FM_WRITE | USB_FM_COVER);
		if(usb_fd == 0x42)
		{
			//todo: �ļ�������Ĵ���Ҫ����
			usb_fd = 0;
			delay_ms(5);
			continue;
		}
		if(p_run->copy_file_type == 0)
			sprintf(copy_buf,"ͨ����,����,ʱ��,ֵ\r\n");
		else if(p_run->copy_file_type == 3)
		{
			sprintf(copy_buf,"����,ʱ��,����\r\n");
			LOG_Set_read_position(0);
		}
		else 
			sprintf(copy_buf,"ͨ����,�¼�����,��������,����ʱ��,��������,����ʱ��\r\n");
		
		USB_Write_file(usb_fd, copy_buf, strlen(copy_buf));
	}
	//��������
		if(p_run->copy_file_type == 0)
			dbp_count_bytes = DBP_Copy_chn_data(usb_fd);
		else if(p_run->copy_file_type == 1)
			dbp_count_bytes = DBP_Copy_chn_alarm(usb_fd);
		else if(p_run->copy_file_type == 2)
			dbp_count_bytes = DBP_Copy_lost_power(usb_fd);
		else if(p_run->copy_file_type == 3)
			dbp_count_bytes = DBP_Copy_log(usb_fd);
	
	delay_ms(500);		//�����һ�θ��½���������ʾ
	if(usb_fd > 0)
	{
		USB_Colse_file(usb_fd);
		//�ָ��������ã�Ӧ�ò�ֻ�ǻָ�ϵͳ���ã�����ͨ�����õȣ�Ӧ��ҲҪ�ָ�
		if(dbp_count_bytes > 10240)
			sprintf(p_run->tip_buf,"�ɹ�д��:%dkB", dbp_count_bytes / 1024);
		else
			sprintf(p_run->tip_buf,"�ɹ�д��:%dB", dbp_count_bytes);
		Win_content(p_run->tip_buf);
		g_DBP_strategy.cmd_hdl(g_DBP_strategy.p_cmd_rcv, sycmd_win_tips, NULL);
		
	}
	
	exit:
	Cmd_del_recv(p_run->arr_DBP_fds[2]);
	
	
}


////������ʼλ��,����
////������ֹʱ�������
//static int DBP_Filtrate_by_time(data_in_fsh_t *src_data, int num_data, uint32_t start_sec, data_in_fsh_t **out_start)
//{
//	int num;
//	short s,e;
//	
//	num = 0;
//	//�ҵ���ʼλ��
//	for(s = 0; s < num_data; s++)
//	{
//		if(src_data[s].rcd_time_s >= start_sec)
//			break;
//		
//	}
//	
//	if(s == num_data)
//		goto exit;
//	*out_start = src_data + s;
//	
//	num = e - s + 1;
//exit:
//	return -1;
//}
//���ؿ������ֽ���

static uint32_t DBP_Copy_chn_data(int fd)
{
	#define DBP_RETRY	3
	uint32_t			start_sec;
	uint32_t			set_start;
	uint32_t			end_sec;
	uint32_t			total;
	uint32_t			done = 0;
	uint32_t			min_sec, max_sec;
	struct  tm			t;
	
	Progress_bar		*p_bar;
	char				*copy_buf;
	data_in_fsh_t		*d;
	dbp_run_t 			*p_run = (dbp_run_t *)arr_p_vram[STG_RUN_VRAM_NUM];

	int					rd_len = 0;
	uint32_t			dbp_count_bytes = 0;
	uint8_t				last_prc = 0;
	uint8_t				prc = 0;
	uint8_t				chn_prc = 0;
	uint8_t				copy_num_chn ;
	uint8_t				copy_chn = DBP_FIRST_CHN;
	uint8_t				done_chn = 0;		//ͨ�����ܲ��Ǵ�0��ʼ������Ҫһ��ר�ŵ���ɼ�����
	uint8_t				retry = DBP_RETRY;
	uint8_t				num_rcd;
	uint8_t				i;
	uint8_t				err;
	
	p_bar = PGB_Get_Sington();
	copy_buf = p_run->usb_buf;
	copy_num_chn = DBP_LAST_CHN - DBP_FIRST_CHN  + 1;
	set_start = Str_time_2_u32(arr_p_vram[row_start_time]);
	end_sec = Str_time_2_u32(arr_p_vram[row_end_time]);
	total = end_sec - set_start + 1;
	
	
	if(set_start == 0xffffffff)
		return 0;
	
	while(done_chn < copy_num_chn)
	{
		
		
		start_sec = set_start;
		chn_prc = done_chn * 100 / copy_num_chn;
		
		//ִ����������䣬�ļ��Ķ�ȡλ����Ȼ�ͻ��ƶ�����Ӧ��λ�ø���
		done = STG_Read_data_by_time(copy_chn, start_sec - 1, 0, (data_in_fsh_t *)copy_buf);	
		STG_Set_file_position(STG_CHN_DATA(copy_chn), STG_DRC_READ, done * sizeof(data_in_fsh_t));
		done = 0;
		
		
		retry = DBP_RETRY;
		err = 0;

		while(done < total)
		{
			if(p_run->DBP_copy == 0)
				break;
			if(phn_sys.usb_device == 0)
				break;
				
			
			//��ȡ����
//			re_read:
			rd_len = STG_Read_rcd(copy_chn, p_run->rcd_buf, sizeof(p_run->rcd_buf));
			if(rd_len <= 0)
			{
				if(retry)
				{
					retry --;
					delay_ms(100);
					continue;
				}
				done = total;
			}
			
			//������ֹʱ��ɸѡ����
			
			num_rcd = rd_len / sizeof(data_in_fsh_t);
			d = (data_in_fsh_t *)p_run->rcd_buf;
			
			for(i = 0; i < num_rcd; i++)
			{
				if(d[i].rcd_time_s != 0xffffffff)
					continue;
				
				err ++;
				
				
			}
			
			//����ʼʱ����ɸѡ����
			for(i = 0; i < num_rcd; i++)
			{
				
				if(d[i].rcd_time_s >= start_sec)
					break;
				
			}
			if(i == num_rcd)
				continue;	//û�з�����ʼʱ�������
			

			
			//������תΪ������ַ�����ʽ
			//todo:��ʱ������usb_buf�Ų��¼�¼���ݵ����
			//Ҫ���ǵ�������ͬһ���ж����¼����˲��ܼ򵥵��Լ�,Ҫ�����һ����¼��ʱ���ȥ����ļ�¼
			min_sec =  d[i].rcd_time_s;
			max_sec = 0;
			rd_len = 0;
			for(;i < num_rcd; i++)
			{
				
				
				if(d[i].rcd_time_s > end_sec)
					continue;
				if(d[i].rcd_time_s < start_sec)
					continue;		//��Ӧ�ó���������,�������ʱ��仯�Ļ����Ϳ��ܳ���������⣬��ʱ����
				
				if(min_sec > d[i].rcd_time_s)
					min_sec = d[i].rcd_time_s;

				
				if(max_sec < d[i].rcd_time_s)
					max_sec = d[i].rcd_time_s;
				
				Sec_2_tm(d[i].rcd_time_s, &t);
				
				sprintf(p_run->usb_buf + rd_len, "%d,%2d/%02d/%02d,%02d:%02d:%02d,", \
				 copy_chn, t.tm_year,t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
				
				
//				if(d[i].decimal_places == 0)
				{
					Print_float(d[i].rcd_val, 0, 0, p_run->tip_buf);
				}
//				else
//				{
//					Print_float(d[i].rcd_val, 0, 1, p_run->tip_buf);
//					
//				}
				strcat(p_run->usb_buf, p_run->tip_buf);
				strcat(p_run->usb_buf, "\r\n");
				rd_len = strlen(p_run->usb_buf);	
			}
			
			
			
			
			//���ַ��������U���ļ�
			
//			rd_len = STG_Read_rcd_by_time(copy_chn, start_sec, end_sec, copy_buf, BDP_USB_BUF_SIZE, &rd_sec);
//			if(rd_len <= 0)
//			{
//				if(retry)
//				{
//					retry --;
//					delay_ms(100);
//					continue;
//				}
//				done = total;
//			}
//			else
//			{
			
			retry = DBP_RETRY;
			done += max_sec - start_sec;
			
			start_sec = min_sec;

			rd_len = strlen(p_run->usb_buf);		
			USB_Write_file(fd, copy_buf, rd_len);
			dbp_count_bytes += rd_len;
					
//			}
			
			
			//�ö�ȡ��ʱ������ʱ��ı�ֵ��Ϊ��������
//		copy_wait:

			prc = done * 100 / total ;
			prc /= copy_num_chn; 
			
			prc += chn_prc;
			if(prc > last_prc)
			{
				p_bar->update_bar(p_run->arr_DBP_fds[1], prc);
				last_prc = prc;
				
			}
			//���⿽��ռ��̫��������ʱ���Ӱ�������̵߳Ĵ洢
			osThreadYield ();  		// suspend thread
		}	//while(done < total)
		copy_chn ++;
		done_chn ++;
	}
	return dbp_count_bytes;
}

static uint32_t DBP_Copy_chn_alarm(int fd)
{
	
	dbp_run_t 			*p_run;
	uint32_t			num;
	uint32_t			start = 0;
	uint32_t			total;
	uint32_t			done = 0;

	
	Progress_bar		*p_bar = PGB_Get_Sington();
	char				*copy_buf;
	int					rd_len = 0;
	uint32_t			dbp_count_bytes = 0;
	uint8_t				last_prc = 0, prc = 0;
	uint8_t				copy_num_chn ;
	uint8_t				copy_chn ;
	uint8_t				done_chn = 1;		//ͨ�����ܲ��Ǵ�0��ʼ������Ҫһ��ר�ŵ���ɼ�����

	
	p_run = STG_P_RUN
	copy_buf = p_run->usb_buf;
	copy_num_chn = DBP_LAST_CHN - DBP_FIRST_CHN  + 1;
	copy_chn = DBP_FIRST_CHN;
	
	while(done_chn <= copy_num_chn)
	{
		
		done = 0;
		start = 0;
		
			
		total = STG_MAX_NUM_CHNALARM;
	
		while(done < total)
		{
			if(p_run->DBP_copy == 0)
				break;
			if(phn_sys.usb_device == 0)
				break;
			
			//��ȡ��ʷ����
			
			//����ʷ����ת�����ַ���
			
			//���ַ���д��usb�ļ�
			
			rd_len = STG_Read_alm_pwr(STG_CHN_ALARM(copy_chn), start, copy_buf, BDP_USB_BUF_SIZE, &num);															
			if(rd_len > 0)
			{
				start += num;
				done += num;
				USB_Write_file(fd, copy_buf, rd_len);
				dbp_count_bytes += rd_len;
			}
			else
			{
				done = total;
				
			}
			
			
			//�ö�ȡ��ʱ������ʱ��ı�ֵ��Ϊ��������
//		copy_wait:

			prc = done * 100 / total ;
			prc /= copy_num_chn; 
			prc += done_chn * 100 / copy_num_chn;

			
			if(prc > last_prc)
			{
				p_bar->update_bar(p_run->arr_DBP_fds[1], prc);
				last_prc = prc;
				
			}			
		}	//while(done < total)
		copy_chn ++;
		done_chn ++;
	}
	
	return dbp_count_bytes;
}

static uint32_t DBP_Copy_lost_power(int fd)
{
	dbp_run_t 			*p_run;
	uint32_t			total ;
	uint32_t			done = 0;
	uint32_t			num;
	uint32_t			count = 0;
	
	Progress_bar		*p_bar = PGB_Get_Sington();
	char				*copy_buf;
	int					rd_len = 0;
	uint32_t			dbp_count_bytes = 0;
	uint8_t				last_prc = 0, prc = 0;
	
	p_run = STG_P_RUN;
	copy_buf = p_run->usb_buf;
	
				
	done = 0;
	count = 0;
	total = STG_MAX_NUM_LST_PWR;
	while(done < total)
	{
		if(p_run->DBP_copy == 0)
			break;
		if(phn_sys.usb_device == 0)
			break;
									
		rd_len = STG_Read_alm_pwr(STG_LOSE_PWR, count, copy_buf, BDP_USB_BUF_SIZE, &num);
		if(rd_len > 0)
		{
			count += num;
			done += num;
			USB_Write_file(fd, copy_buf, rd_len);
			dbp_count_bytes += rd_len;
		}
		else
		{
			done = total;
		}


		//�ö�ȡ��ʱ������ʱ��ı�ֵ��Ϊ��������
//	copy_wait:

		prc = done * 100 / total ;

		if(prc > last_prc)
		{
			p_bar->update_bar(p_run->arr_DBP_fds[1], prc);
			last_prc = prc;
		}			
	}	//while(done < total)
	return dbp_count_bytes;
}

static uint32_t DBP_Copy_log(int fd)
{
	dbp_run_t 			*p_run;
	uint32_t			total;
	uint32_t			done = 0;
	
	Progress_bar		*p_bar = PGB_Get_Sington();
	char				*copy_buf;
	int					rd_len = 0;
	uint32_t			dbp_count_bytes = 0;
	uint8_t				last_prc = 0, prc = 0;
	
	
	p_run = STG_P_RUN;
	copy_buf = p_run->usb_buf;
	done = 0;
	total = LOG_Get_total_num() * sizeof(rcd_log_t);
	while(done < total)
	{
		if(p_run->DBP_copy == 0)
			break;
		if(phn_sys.usb_device == 0)
			break;
		rd_len = LOG_Read(copy_buf, BDP_USB_BUF_SIZE);
		if(rd_len <= 0)
			done = total;
		else
		{
			done = LOG_Get_read_num() * sizeof(rcd_log_t);
			USB_Write_file(fd, copy_buf, rd_len);
			dbp_count_bytes += rd_len;
		}
		//�ö�ȡ��ʱ������ʱ��ı�ֵ��Ϊ��������
//		copy_wait:
		prc = done * 100 / total ;

		if(prc > last_prc)
		{
			p_bar->update_bar(p_run->arr_DBP_fds[1], prc);
			last_prc = prc;

		}			
	}	//while(done < total)

	return dbp_count_bytes;
}

 static void	DBP_Btn_hdl(void *self, uint8_t	btn_id)
 {
	 dbp_run_t 			*p_run = STG_P_RUN;
	 if(btn_id == ICO_ID_COPY)
	 {
		 p_run->DBP_copy = 1;
		 p_run->arr_DBP_fds[2] = Cmd_Rgt_recv(DBP_Copy);		 
	 }
	 else if(btn_id == ICO_ID_STOP)
	 {
		 p_run->DBP_copy = 0;
	 }
	 
 }

