#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <protocol.h>


//请求GET 文件报文构造函数

//获取文件部分数据，如果请求所有数据，则将rangeleft和rangeright都置为0
//要获取指定位置的数据，则置rangeleft和rangeright为相同的断点偏移
//必要参数
//	req_path: 请求文件在目标主机的路径
//	host_target：目标主机（可以是域名，IP带端口或者直接IP）
//	accect_patterns：告诉主机希望请求的数据的类型，如果类型不为空，则服务端判断目标类型不一致可能不会发数据应答
int http_build_get_msg(http_get_req_head_t header,char *sendbuff,int rangeleft,int rangeright)
{
    char line_buffer[160]={0};
    if(header.req_path==NULL||header.host_target==NULL||header.accect_patterns==NULL)
    {
		return -1;
    }

    
    sprintf(sendbuff, "GET %s HTTP/1.1\r\n",header.req_path);

    sprintf(line_buffer,"Host: %s\r\n",header.host_target);
    strcat(sendbuff, line_buffer);

    sprintf(line_buffer,"Accept: %s\r\n",header.accect_patterns);
    strcat(sendbuff, line_buffer);

    if(header.user_agent!=NULL)
    {
        sprintf(line_buffer,"User-Agent: %s\r\n",header.user_agent);
        strcat(sendbuff, line_buffer);
    }
    if(header.connect_mode!=NULL)
    {
        sprintf(line_buffer,"Connection: %s\r\n",header.connect_mode);
        strcat(sendbuff, line_buffer);
    }


    char s_range[64]={0};
    if(rangeleft==rangeright)
    {
	if(rangeright)
	{
		sprintf(s_range, "Range: bytes=%d-\r\n",rangeleft);//获取指定断点后数据
		strcat(sendbuff, s_range);
	}
    }
    else
    {
    	sprintf(s_range, "Range: bytes=%d-%d\r\n",rangeleft,rangeright);//获取指定断点后的指定范围的数据
    	strcat(sendbuff, s_range);
    }
    strcat(sendbuff, "\r\n");//newline end
    return 0;

}

//HTTP POST 方法(简单模式)
//客户端向服务端提交数据，
int http_build_post_msg(http_post_req_head_t header,char *sendbuff,int rangeleft,int rangeright)
{
    char line_buffer[256]={0};
    if(header.req_path==NULL||header.host_target==NULL||header.accect_patterns==NULL)
    {
	return -1;
    }
    sprintf(sendbuff, "POST %s HTTP/1.1\r\n",header.req_path);\

    sprintf(line_buffer,"Host: %s\r\n",header.host_target);
    strcat(sendbuff, line_buffer);

    sprintf(line_buffer,"Accept: %s\r\n",header.accect_patterns);
    strcat(sendbuff, line_buffer);

    sprintf(line_buffer,"Referer: %s\r\n",header.referer);
    strcat(sendbuff, line_buffer);

    if(header.user_agent!=NULL)
    {
        sprintf(line_buffer,"User-Agent: %s\r\n",header.user_agent);
        strcat(sendbuff, line_buffer);
    }
    if(header.connect_mode!=NULL)
    {
        sprintf(line_buffer,"Connection: %s\r\n",header.connect_mode);
        strcat(sendbuff, line_buffer);
    }
    strcat(sendbuff, "\r\n");//newline end

    if(header.Ext_params!=NULL)
    {
        strcat(sendbuff, header.Ext_params);
    }
    return 0;
}

//解析HTTP应答头
//主要返回状态码，内容类型以及内容长度，在检测到头结束符（\r\n\r\n）后解析头
http_pesponse_head_t parse_http_header(const char *response)
{
	/*get info from response header*/
    http_pesponse_head_t resp={0};
    char* data_pos =NULL;
    char *pos =strstr(response, "HTTP/");
	DbgFuncEntry();
    if (pos)//get status code
    {
        sscanf(pos, "%*s %d", &resp.status_code);
    }

    pos = strstr(response, "Content-Type:");
    if (pos)//get content type
    {
        sscanf(pos, "%*s %s", resp.content_type);
    }
    pos = strstr(response, "Content-Length:");
    if (pos)//content length
    {
        sscanf(pos, "%*s %ld", &resp.content_length);
    }
	if(resp.status_code==206)
	{
		pos = strstr(response, "Content-Range:");
	    if (pos)//get content type for 206
	    {
	    //Content-Range: bytes 0-1023/623671
	        sscanf(pos, "%*s %s %d-%*d/%ld", resp.Content_Range,&resp.content_offset,&resp.content_length);
	    }
	}
    data_pos = strstr(pos,"\r\n\r\n");
    if(data_pos)
    {
    	resp.content_offset = (data_pos+4)-response;
    }
	DbgFuncExit();
    return resp;
}

int hex2dec(char c)
{
    if ('0' <= c && c <= '9') 
    {
        return c - '0';
    } 
    else if ('a' <= c && c <= 'f')
    {
        return c - 'a' + 10;
    } 
    else if ('A' <= c && c <= 'F')
    {
        return c - 'A' + 10;
    } 
    else 
    {
        return -1;
    }
}

char dec2hex(short int c)
{
    if (0 <= c && c <= 9) 
    {
        return c + '0';
    } 
    else if (10 <= c && c <= 15) 
    {
        return c + 'A' - 10;
    } 
    else 
    {
        return -1;
    }
}

#define BURSIZE 2048

//编码一个url
void urlencode(char url[])
{
    int i = 0;
    int len = strlen(url);
    int res_len = 0;
    char res[BURSIZE];
    for (i = 0; i < len; ++i) 
    {
        char c = url[i];
        if (    ('0' <= c && c <= '9') ||
                ('a' <= c && c <= 'z') ||
                ('A' <= c && c <= 'Z') || 
                c == '/' || c == '.') 
        {
            res[res_len++] = c;
        } 
        else 
        {
            int j = (short int)c;
            if (j < 0)
                j += 256;
            int i1, i0;
            i1 = j / 16;
            i0 = j - i1 * 16;
            res[res_len++] = '%';
            res[res_len++] = dec2hex(i1);
            res[res_len++] = dec2hex(i0);
        }
    }
    res[res_len] = '\0';
    strcpy(url, res);
}

// 解码url
void urldecode(char url[])
{
    int i = 0;
    int len = strlen(url);
    int res_len = 0;
    char res[BURSIZE];
    for (i = 0; i < len; ++i) 
    {
        char c = url[i];
        if (c != '%') 
        {
            res[res_len++] = c;
        }
        else 
        {
            char c1 = url[++i];
            char c0 = url[++i];
            int num = 0;
            num = hex2dec(c1) * 16 + hex2dec(c0);
            res[res_len++] = num;
        }
    }
    res[res_len] = '\0';
    strcpy(url, res);
}

void parse_url_ext(const char *url, char *host, int *port, char *file_name,char* param_buffer)
{
	/*parsing url, and get host, port and file name*/
    int i;
    int j = 0;
    int start = 0;
    *port = 80;
    char *patterns[] = {"http://", "https://", "ftp://", NULL};

    for (i = 0; patterns[i]; i++)//remove http and https
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
            start = strlen(patterns[i]);

    for (i = start; url[i] != '/' && url[i] != '\0'; i++, j++)
        host[j] = url[i];
    host[j] = '\0';

	//get port, defalt is 80
    char *pos = strstr(host, ":");
    if (pos)
    {
        sscanf(pos, ":%d", port);
    }
	for (i = 0; i < (int)strlen(host); i++)
	{
		if (host[i] == ':')
		{
			host[i] = '\0';
			break;
		}
	}

	//get file name
    j = 0;
    for (i = start; url[i] != '\0'&&url[i] != '?'&&url[i] != '&'; i++)
    {
        if (url[i] == '/')
        {
            if (i !=  strlen(url) - 1)
                j = 0;
            continue;
        }
        else
            file_name[j++] = url[i];
    }
    file_name[j] = '\0';
    if(param_buffer!=NULL && url[i]=='&')
    {
        rt_kprintf("get param in url\r\n");
        j = 0;
        for (; url[i] != '\0'; i++)//continue
        {
            param_buffer[j++] = url[i];
        }  
        param_buffer[j] = '\0';    
    }
}

//解析HTTP的URL
void parse_url(const char *url, char *host, int *port, char *file_name)
{
    parse_url_ext(url, host, port, file_name,NULL);
}


bool IsaIPv4(const char *psz)
{
    if(strlen(psz) > 15 || strlen(psz) < 7) return false;
    int iPoint = 0;
    const char *b = psz;
    while(*psz)
    {
        if(*psz == '.') 
        {
            if(psz-b < 1) return false;
            if(psz-b > 1 && *b == '0') return false;
            if(atoi(b) > 255) return false;
            iPoint++;
            b = psz+1;
        }
        else if(!isdigit(*psz)) return false;
        psz++;
    }
    if(iPoint != 3) return false;
    if(psz-b < 1) return false;
    if(psz-b > 1 && *b == '0') return false;
    if(atoi(b) > 255) return false;

    return true; 
}


