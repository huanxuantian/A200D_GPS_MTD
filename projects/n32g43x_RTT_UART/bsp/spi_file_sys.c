#include <rtthread.h>
#define DBG_LEVEL           DBG_INFO
#define DBG_SECTION_NAME    "SPI_SYS"
#include <rtdbg.h>
#if 1//def USE_SPI_SYS
#include <string.h>
#include "utils.h"
#include <system_bsp.h>
#endif
#include "spi_file_sys.h"

static int spi_flash_vaild_flag=0;

void update_flash_flag(int flag)
{
	spi_flash_vaild_flag = flag;
}

int is_flash_vaild()
{
	return spi_flash_vaild_flag;
}

static int spi_fs_control(spi_file_fd fd,spi_file_control_cmd cmd,rt_uint8_t* data);

int build_spi_fs_fd(spi_file_fd fd,rt_uint32_t Saddress,int file_size){
    //chech address ok and fix to block size;
    fd->Saddress = Saddress;
    fd->fsize = file_size;
    //calc head size first(now default to fs_base_head_t only)
    //calc real block need for write and read;
    fd->sector_count = (file_size + sizeof(fs_base_head_t))/SECTOR_SIZE +((file_size + sizeof(fs_base_head_t))%SECTOR_SIZE>0)?1:0;
	return 1;
}

static uint8_t init_falg=0;
static void build_fd_header(spi_file_fd fd)
{
	memcpy(fd->header.fs_flag,SPI_FS_FLAG,MIN(strlen(SPI_FS_FLAG),sizeof(fd->header.fs_flag)));
	fd->header.fs_version = SPI_FS_VERSION;
	fd->header.data_address = fd->Saddress+sizeof(fs_base_head_t);
	fd->header.fsize = fd->fsize;
}

int spi_fs_open(spi_file_fd fd,spi_file_mode mode){
    
    //read file head for fd handle
    //set mode and init flag and base offset
	if(!is_flash_vaild())
	{
		return -1;
	}
    if(fd==NULL){return -1;}
    fd->mode = mode;
    //if(fd->Saddress >= FS_MAX_ADDRESS|| fd->sector_count <= 0){return -2;}
    //if(fd->Saddress + fd->sector_count*SECTOR_SIZE > FS_MAX_ADDRESS){return -2;}
    if(fd->state>0) {return -5;}
    fd->roffset =0;
    fd->woffset=0;
    fd->storage_dev = rt_device_find("Flash");
	if(fd->storage_dev==RT_NULL) return -1;
	if(init_falg==0)
	{
		init_falg=1;
	}
	spi_fs_control(fd,SPI_FS_INIT_EMPTY_HEAD,NULL);
	rt_device_read(fd->storage_dev, fd->Saddress,&fd->header,sizeof(fs_base_head_t));
	fd->state=1;
	return 1;
}

int spi_fs_close(spi_file_fd fd){

    if(fd==NULL){return -1;}
    if(fd->state<=0) {return -5;}
    //clean fd only
    //spi_fs_control(fd,SPI_FS_INIT_EMPTY_HEAD,NULL);
    //flush the head data 
	fd->roffset =0;
	fd->woffset=0;
	fd->state=0;
	fd->mode = SPI_FS_MODE_NULL;
	//memset(&fd->header,0,sizeof(fs_base_head_t));
	return 1;
}
#ifdef USE_SPI_SYS
int spi_fs_write(spi_file_fd fd,rt_uint8_t* data,rt_int32_t data_size){
    rt_size_t ret=0;

    uint32_t data_address;
	uint32_t sectoraddr;
#ifdef USE_RECOVER_BLOCK
	int i=0;
	int j=0;
	uint32_t sector_head_offset;
	uint32_t sector_data_offset;
	uint8_t* sfs_buff=RT_NULL;
#endif
	if(!is_flash_vaild())
	{
		return -1;
	}
    if(fd==NULL){return -1;}
    if(fd->state<=0 || fd->mode ==SPI_FS_MODE_RO) {return -2;}
    //if(fd->storage_dev==RT_NULL) {return -3;}
	fd->storage_dev = rt_device_find("Flash");
	if(fd->storage_dev==RT_NULL) return -3;
    //check open state and head state
    //spi_fs_control(fd,SPI_FS_INIT_EMPTY_HEAD,NULL);
    //memcmp head_flag init head data here
    //calc real data address for write
    data_address = fd->Saddress + sizeof(fs_base_head_t);
	//check blank or not 
	sectoraddr = ((int)(fd->Saddress/SECTOR_SIZE))*SECTOR_SIZE;
	#ifdef USE_RECOVER_BLOCK
	sector_head_offset = fd->Saddress%SECTOR_SIZE;
	sector_data_offset = sector_head_offset + sizeof(fs_base_head_t);
	if(sector_data_offset > SECTOR_SIZE){
		return -3;
	}
	#endif
	//
	build_fd_header(fd);
	#ifdef USE_RECOVER_BLOCK
	sfs_buff = rt_malloc(SECTOR_SIZE);
	if(sfs_buff!=RT_NULL)
	{
		memset(sfs_buff,0,SECTOR_SIZE);
		ret = rt_device_read(fd->storage_dev,sectoraddr,sfs_buff,SECTOR_SIZE);
		j=0;
		for(i=0;i<sizeof(sfs_buff);i++){
			if(sfs_buff[i]!=0xFF){
				break;
			}
			j++;
		}
		
		if(j<i || j<SECTOR_SIZE){
			LOG_I("sectoraddr(0x%06X) earse rewrite now!!!",sectoraddr);
			//write data to flash,earse first
			rt_device_control(fd->storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
			rt_thread_mdelay(20);
			//flush update
			memcpy(sfs_buff+sector_head_offset,&fd->header,sizeof(fs_base_head_t));
			memcpy(sfs_buff+sector_data_offset,data,data_size);
			ret = rt_device_write(fd->storage_dev,sectoraddr,sfs_buff,SECTOR_SIZE);
			LOG_I("flash sector data write:");
			debug_print_block_data(sectoraddr,sfs_buff, SECTOR_SIZE);
		}
		else
		{
			//empty sector update only
			ret = rt_device_write(fd->storage_dev,fd->Saddress,&fd->header,sizeof(fs_base_head_t));
			LOG_I("flash header write:");
			debug_print_block_data(fd->Saddress,(uint8_t*)&fd->header, sizeof(fs_base_head_t));
			rt_thread_mdelay(10);
			ret = rt_device_write(fd->storage_dev,data_address,data,data_size);
			LOG_I("flash data write:");
			debug_print_block_data(data_address,data, data_size);
		}
	}
	else
	#endif
	{
		rt_device_control(fd->storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
		rt_thread_mdelay(20);
		//empty sector update only
		ret = rt_device_write(fd->storage_dev,data_address,data,data_size);
		LOG_I("flash data write:");
		debug_print_block_data(data_address,data, data_size);
		rt_thread_mdelay(10);
		ret = rt_device_write(fd->storage_dev,fd->Saddress,&fd->header,sizeof(fs_base_head_t));
		LOG_I("flash header write:");
		debug_print_block_data(fd->Saddress,(uint8_t*)&fd->header, sizeof(fs_base_head_t));
	}
	#ifdef USE_RECOVER_BLOCK
	if(sfs_buff!=RT_NULL){
		rt_free(sfs_buff);
	}
	#endif

    //update fs_head and write to head info
    //update woffset for mode policy
    return ret;
}
#else
int spi_fs_write(spi_file_fd fd,rt_uint8_t* data,rt_int32_t data_size){
	uint32_t data_address;
	uint32_t sectoraddr;
	int ret = 0;
	fd->storage_dev = rt_device_find("Flash");
	if(fd->storage_dev==RT_NULL) return -1;
	if(fd->state<=0 || fd->mode ==SPI_FS_MODE_RO) {return -2;}
	data_address = fd->Saddress + sizeof(fs_base_head_t);
	sectoraddr = ((int)(fd->Saddress/SECTOR_SIZE))*SECTOR_SIZE;
	//build header
	build_fd_header(fd);
	//erase 
	rt_device_control(fd->storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
	rt_thread_mdelay(20);
	//write_data
	ret = rt_device_write(fd->storage_dev,data_address,(unsigned char*)data,data_size);
	LOG_I("flash data write:");
	debug_print_block_data(data_address,(unsigned char*)data,data_size);
	rt_thread_mdelay(10);
	//write_header
	ret = rt_device_write(fd->storage_dev,fd->Saddress,&fd->header,sizeof(fs_base_head_t));
	LOG_I("flash header write:");
	debug_print_block_data(fd->Saddress,(unsigned char*)&fd->header, sizeof(fs_base_head_t));

	return ret;
}
#endif
int spi_fs_read(spi_file_fd fd,rt_uint8_t* buff,rt_int32_t buff_size){
    rt_size_t ret=0;
    uint32_t data_address;
	if(!is_flash_vaild())
	{
		return -1;
	}
    if(fd==NULL){return -1;}
    if(fd->state<=0) {return -2;}
    //if(fd->storage_dev==RT_NULL) {return -3;}
	fd->storage_dev = rt_device_find("Flash");
	if(fd->storage_dev==RT_NULL) return -1;
    //check open state and head state
    //spi_fs_control(fd,SPI_FS_INIT_EMPTY_HEAD,NULL);
    //calc real data address for read
    data_address = fd->Saddress + sizeof(fs_base_head_t);
    //read data from flash
	ret = rt_device_read(fd->storage_dev, fd->Saddress,&fd->header,sizeof(fs_base_head_t));
	if(ret<=0){
		DbgError(" read file header Error!!!!!");
		return -1;
	}
	if(fd->header.fsize >0){
		LOG_I("header fsize:%d",fd->header.fsize);
	}
	rt_thread_mdelay(2);
    ret = rt_device_read(fd->storage_dev, data_address, buff, buff_size);
	LOG_I("header read:");
	debug_print_block_data(fd->Saddress,(uint8_t*)&fd->header,sizeof(fs_base_head_t));
	LOG_I("flash data read:");
	debug_print_block_data(data_address,buff, buff_size);
    //update roffset for mode policy
    return ret;
}

static int spi_fs_control(spi_file_fd fd,spi_file_control_cmd cmd,rt_uint8_t* data){

	if(fd==NULL){return -1;}
    //check cmd for control
    //detect the struct for each control data
    //handle data or update fd control flag only
    switch(cmd){
        case SPI_FS_GETFSIZE:
            break;
        case SPI_FS_INIT_EMPTY_HEAD:
			memset(&fd->header,0,sizeof(fs_base_head_t));
            break;
        default:
            break;
    }
	return 0;
}
