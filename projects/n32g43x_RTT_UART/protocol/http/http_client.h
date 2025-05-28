#ifndef _HTTP_CLIENT_H
#define	_HTTP_CLIENT_H
#ifdef __cplusplus
extern "C" {
#endif

#define	HTTP_HEAD_STA_STR 	"HTTP/"

#define HTTP_HEAD_END_STR	"\r\n\r\n"

//HTTP请求报文消息基本内容结构
typedef struct
{
    char* req_path;//GET %s HTTP/1.1 {"/test/test.html"}
    char* accect_patterns;//Accept: %s {""}
    char* user_agent;//User-Agent: %s {"shinki_downloader"}
    char* host_target;//Host: %s {"192.168.0.94"}
    char* connect_mode;//Connection: %s {"keep-alive"}
}http_get_req_head_t;


//HTTP POST
typedef struct
{
    char* req_path;//POST %s HTTP/1.1 {"/test"}
    char* accect_patterns;//Accept: %s {""}
    char* referer;//Referer: %s
    char* user_agent;//User-Agent: %s {"shinki_downloader"}
    char* host_target;//Host: %s {"192.168.0.94"}
    char* connect_mode;//Connection: %s {"keep-alive"}
    char* Ext_params;//data for post
}http_post_req_head_t;

//HTTP头应答报文基本内容结构
typedef struct//record some info of http response header
{
    int status_code;//HTTP/1.1 '200' OK //	HTTP/1.1 206 Partial Content
    char content_type[128];//Content-Type: application/gzip
    long content_length;//Content-Length: 11683079
    char Accept_Ranges[64];//Accept-Ranges: bytes
    char Content_Range[128];//Content-Range: bytes 0-1023/623671
    int  content_offset;//
}http_pesponse_head_t;


//请求GET 文件报文构造函数

//获取文件部分数据，如果请求所有数据，则将rangeleft和rangeright都置为0
//要获取指定位置的数据，则置rangeleft和rangeright为相同的断点偏移
//必要参数
//	req_path: 请求文件在目标主机的路径
//	host_target：目标主机（可以是域名，IP带端口或者直接IP）
//	accect_patterns：告诉主机希望请求的数据的类型，如果类型不为空，则服务端判断目标类型不一致可能不会发数据应答
int http_build_get_msg(http_get_req_head_t header,char *sendbuff,int rangeleft,int rangeright);

//HTTP POST 方法(简单模式)
//客户端向服务端提交数据，
int http_build_post_msg(http_post_req_head_t header,char *sendbuff,int rangeleft,int rangeright);

//解析HTTP应答头
//主要返回状态码，内容类型以及内容长度，在检测到头结束符（\r\n\r\n）后解析头
http_pesponse_head_t parse_http_header(const char *response);

void urlencode(char url[]);
void urldecode(char url[]);

void parse_url_ext(const char *url, char *host, int *port, char *file_name,char* param_buffer);
void parse_url(const char *url, char *host, int *port, char *file_name);

bool IsaIPv4(const char *psz);

int get_ip_addr(const char *host_name, char *ip_addr);

#ifdef __cplusplus   
} 
#endif

#endif

