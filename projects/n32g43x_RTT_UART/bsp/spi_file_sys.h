#ifndef _SPI_FILE_SYS_H
#define	_SPI_FILE_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

#define SECTOR_SIZE 4096

#define FS_MAX_ADDRESS 0x100000

#define SPI_FS_FLAG "FSPI"
#define SPI_FS_VERSION  0x200

typedef struct{
    rt_uint8_t fs_flag[4];
    rt_uint16_t fs_version;
    rt_uint16_t head_ext_flag;
    rt_uint32_t data_address;
    rt_uint32_t fsize;
}fs_base_head_t;

typedef struct{
    rt_device_t storage_dev;
    rt_uint32_t Saddress;
    rt_uint32_t sector_count;
    rt_uint32_t fsize;
    rt_uint8_t mode;
    rt_uint8_t state;
    rt_uint8_t reserve[2];
    rt_uint32_t woffset;
    rt_uint32_t roffset;
    fs_base_head_t header;
}spi_file_s;

typedef spi_file_s* spi_file_fd;

typedef enum{
    SPI_FS_MODE_NULL=0,//test check file and init block only
    SPI_FS_MODE_RO,
    SPI_FS_MODE_WO,
    SPI_FS_MODE_RW
}spi_file_mode;

typedef enum{
    SPI_FS_GETFSIZE=0,
    SPI_FS_SETFSIZE,
    SPI_FS_SET_ROFFSET,
    SPI_FS_GET_ROFFSET,
    SPI_FS_SET_WOFFSET,
    SPI_FS_GET_WOFFSET,
    SPI_FS_INIT_BLOCK,
    SPI_FS_INIT_EMPTY_HEAD
}spi_file_control_cmd;



int build_spi_fs_fd(spi_file_fd fd,rt_uint32_t Saddress,int file_size);

int spi_fs_open(spi_file_fd fd,spi_file_mode mode);

int spi_fs_close(spi_file_fd fd);

int spi_fs_write(spi_file_fd fd,rt_uint8_t* data,rt_int32_t data_size);
int spi_fs_read(spi_file_fd fd,rt_uint8_t* buff,rt_int32_t buff_size);




#ifdef __cplusplus
}
#endif

#endif