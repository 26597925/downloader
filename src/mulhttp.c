/*
 *
 * Copyright (c) wenford.li
 * wenford.li <26597925@qq.com>
 *
 */
#include <stdio.h>
#include "mulhttp.h"

#define NSECTOSEC 1000000000
#define DEUA "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2922.1 Safari/537.36"
 
struct http_header {
  char 	 *memory;
  size_t size;
};
 
typedef struct threadinfo {
	
	long start_pos;  //下载起始位置
	long end_pos;	   //下载结束位置
	long block_size; //本线程负责下载的数据大小
	long recv_size; //本线程已经接收到的数据大小
	
	char *memory;	
	CURL *curl;
	mulhttp_t *mulhttp; 
	
} threadinfo_t;

static size_t header_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	
	size_t realsize = size * nmemb;
	struct http_header *mem = (struct http_header *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	
	size_t realsize = size * nmemb;
	threadinfo_t *threadinfo = (threadinfo_t *) userp;
	threadinfo->memory = realloc(threadinfo->memory, threadinfo->recv_size + realsize + 1);
	memcpy(&(threadinfo->memory[threadinfo->recv_size]), contents, realsize);
	threadinfo->recv_size += realsize;
	threadinfo->memory[threadinfo->recv_size] = 0;
	
	return realsize;
}

static void *task_loop(void *arg) 
{
	threadinfo_t *threadinfo = (threadinfo_t *) arg;
	
	CURLcode res;
	res = curl_easy_perform(threadinfo->curl);
	
	long retcode = 0;
	res = curl_easy_getinfo(threadinfo->curl, CURLINFO_RESPONSE_CODE , &retcode);
		
	curl_easy_cleanup(threadinfo->curl);
	
	mulhttp_t *mulhttp = threadinfo->mulhttp;
	
	mulhttp->flags++;
		
	mulhttp->memory = realloc(mulhttp->memory, threadinfo->recv_size + threadinfo->start_pos);
	memcpy(&(mulhttp->memory[threadinfo->start_pos]), threadinfo->memory, threadinfo->recv_size);
	
	if(mulhttp->flags == mulhttp->thread_num)
	{
		sem_post(&mulhttp->sem);	
	}
	
	return 0;
}

static void start_http(mulhttp_t *mulhttp, threadinfo_t *threadinfo)
{
	CURL *curl = curl_easy_init();
	if (curl == NULL)  
    {
        curl_global_cleanup();   
        return NULL;  
    }
	
	threadinfo->curl = curl;
	
	char range[64] = { 0 };
    snprintf (range, sizeof (range), "%ld-%ld", threadinfo->start_pos, threadinfo->end_pos);
	
	curl_easy_setopt(threadinfo->curl, CURLOPT_URL, mulhttp->url);
	curl_easy_setopt(threadinfo->curl, CURLOPT_HTTPHEADER, mulhttp->headers);
	curl_easy_setopt(threadinfo->curl, CURLOPT_HEADER, 0);//设置非0则，输出包含头信息
	curl_easy_setopt(threadinfo->curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(threadinfo->curl, CURLOPT_CONNECTTIMEOUT, mulhttp->timeout);
	curl_easy_setopt(threadinfo->curl, CURLOPT_TIMEOUT, mulhttp->timeout);
	curl_easy_setopt(threadinfo->curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(threadinfo->curl, CURLOPT_WRITEDATA, (void *)threadinfo);
	curl_easy_setopt(threadinfo->curl, CURLOPT_FOLLOWLOCATION, mulhttp->location);
	curl_easy_setopt(threadinfo->curl, CURLOPT_MAXREDIRS, 5);
	curl_easy_setopt(threadinfo->curl, CURLOPT_RANGE, range);
	
	threadpool_add_job(mulhttp->thread__pool, task_loop, threadinfo);
}

static void get_download_info(mulhttp_t *mulhttp)
{
	struct http_header header;
	header.memory = malloc(1);
	header.size = 0;
	
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, mulhttp->url);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, mulhttp->headers);
	curl_easy_setopt(handle, CURLOPT_HEADER, 1);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
	curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);
	CURLcode res = curl_easy_perform(handle);
	if (res == CURLE_OK) {
		curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &mulhttp->content_size);
		
		if(strstr(header.memory, "Accept-Ranges: bytes"))
		{
			mulhttp->is_range = 1;
		}
		
	}
	curl_easy_cleanup(handle);
}

mulhttp_t *init_mulhttp(char *url)
{
	mulhttp_t *mulhttp = (mulhttp_t *)malloc(sizeof(mulhttp_t));
	mulhttp->url = url;
	mulhttp->headers == NULL;
	mulhttp->is_range = 0;
	mulhttp->content_size = 0;
	mulhttp->thread_num = 4;
	mulhttp->timeout = 5;
	mulhttp->location = 1;
	mulhttp->flags = 0;
	mulhttp->memory = malloc(1);
	mulhttp->thread__pool = threadpool_init(mulhttp->thread_num, 100);
	sem_init(&mulhttp->sem, 0, 0);
	
	if(mulhttp->headers == NULL)
	{
		add_mul_header(mulhttp, DEUA);
	}
	
	get_download_info(mulhttp);
	
	return mulhttp;
}

void add_mul_header(mulhttp_t *mulhttp, char *header)
{
	mulhttp->headers = curl_slist_append(mulhttp->headers, header);
}

void set_mul_time_out(mulhttp_t *mulhttp, int tout)
{
	mulhttp->timeout = tout;
}

void set_mul_follow_location(mulhttp_t *mulhttp, int flocation)
{
	mulhttp->location = flocation;
}

int start_download(mulhttp_t *mulhttp)
{
	int ret = -1;
	
	if(mulhttp->content_size == 0 
		|| mulhttp->is_range == 0)
	{
		return ret;
	}
	
	long part_size =  mulhttp->content_size/mulhttp->thread_num;
	
	int i;
	for (i = 0; i < mulhttp->thread_num; i++)  
    {  
		threadinfo_t *threadinfo = (threadinfo_t *)malloc(sizeof(threadinfo_t));
		threadinfo->mulhttp = mulhttp;
		threadinfo->recv_size = 0;
		threadinfo->memory = malloc(1);
		
		if (i < (mulhttp->thread_num - 1))  
        {  
            threadinfo->start_pos = i * part_size;  
            threadinfo->end_pos = (i + 1) * part_size - 1;
        }  
        else  
        {   
            threadinfo->start_pos = i * part_size;  
            threadinfo->end_pos = mulhttp->content_size - 1;  
        }
		
		threadinfo->block_size = threadinfo->end_pos - threadinfo->start_pos + 1;
		
		start_http(mulhttp, threadinfo);
	}
	
	ret = 1;
	
	printf("%s, %s, %d  \n", __FILE__, __FUNCTION__, __LINE__);
	
	return ret;
}

void stop_download(mulhttp_t *mulhttp)
{
	threadpool_destroy(mulhttp->thread__pool);
	
	if(mulhttp->headers != NULL) 
		curl_slist_free_all(mulhttp->headers);
	
	sem_destroy(&mulhttp->sem);
}

void wait_content(mulhttp_t *mulhttp)
{
	struct timespec ts;
	ts.tv_sec = time(NULL) + mulhttp->timeout;
	ts.tv_nsec = 0;
    sem_timedwait(&mulhttp->sem, &ts);
}

void write_file(mulhttp_t *mulhttp, char *path)
{
	FILE *stream;
	stream = fopen(path, "w");
	fwrite(mulhttp->memory, 1, mulhttp->content_size, stream);
	fclose(stream);
}
