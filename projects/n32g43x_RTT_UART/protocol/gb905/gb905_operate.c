#include <rtthread.h>

#include "utils.h"
#include <system_bsp.h>
#include <business.h>
#include <protocol.h>

static gb905_operate_report_t operate_report_data;

void gb905_build_pre_operate_start()
{
    memset(&operate_report_data,0,sizeof(gb905_operate_report_t));
    gb905_build_report(&operate_report_data.pre_operate.begin_report);
   operate_report_data.pre_operate.operation_id.id = EndianReverse32(gb905_build_new_timestamp_id());

}

void gb905_send_operate(gb2014_meter_operation_t* operate)
{
    int msg_len=0;
    gb905_build_report(&operate_report_data.pre_operate.end_report);
    operate_report_data.operate = *operate;

    msg_len = sizeof(operate_t)+sizeof(gb2014_meter_operation_t);
    
    gb905_build_header(&operate_report_data.header,MESSAGE_OPERATION_REPORT,msg_len);

    debug_print_block_data(0,(unsigned char*)&operate_report_data,sizeof(gb905_operate_report_t));

    //

}

void gb905_operate_check(unsigned char index,int clrflag)
{
    if(operate_report_data.operate.board_time_bcd[0]!=0x00)
    {
        gb905_send_data(index,(unsigned char*)&operate_report_data,sizeof(gb905_operate_report_t));
        if(clrflag)
        {
            memset(&operate_report_data,0,sizeof(gb905_operate_report_t));
        }
    }
    
}

