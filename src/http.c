/*
 *
 * Copyright (c) wenford.li
 * wenford.li <26597925@qq.com>
 *
 */
#include "http.h"

http_t *request(char *url)
{
	CURL *curl = curl_easy_init();
	if (curl == NULL)  
    {
        curl_global_cleanup();   
        return NULL;  
    }
	printf("%s, %s, %d, %s\n", __FILE__,__FUNCTION__,__LINE__, url);
	
	curl_easy_setopt(curl, CURLOPT_URL, url);
	
	http_t *http = (http_t *)malloc(sizeof(http_t));
	http->curl = curl;
	http->headers = NULL;
	http->timeout = 1;
	http->location = 1;
	
	return http;
}

void add_header(http_t *http, char *header)
{
	http->headers = curl_slist_append(http->headers, header);
}

void add_params(http_t *http, char *param)
{
	curl_easy_setopt(http->curl, CURLOPT_POSTFIELDS, param);
    curl_easy_setopt(http->curl, CURLOPT_POSTFIELDSIZE, sizeof(param));
}

void add_proxy(http_t *http, char *proxy_port, char *proxy_user_password)
{
	curl_easy_setopt(http->curl, CURLOPT_PROXY, proxy_port);
	curl_easy_setopt(http->curl, CURLOPT_PROXYUSERPWD, proxy_user_password);
}

void set_proxy_type(http_t *http, long type)
{
	curl_easy_setopt(http->curl, CURLOPT_PROXYTYPE, type);//CURLPROXY_SOCKS4,默认http
}

void add_cookie(http_t *http, char *cookie)
{
	curl_easy_setopt(http->curl, CURLOPT_COOKIE, cookie);
}

void set_refer(http_t *http, char *refer){
	curl_easy_setopt(http->curl, CURLOPT_REFERER, refer);
}

void set_user_agent(http_t *http, char *use_agent){
	curl_easy_setopt(http->curl, CURLOPT_USERAGENT, use_agent);
}

void set_time_out(http_t *http, int tout)
{
	http->timeout = tout;
}

void set_follow_location(http_t *http, int flocation)
{
	http->location = flocation;
}

static size_t write_data(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct memory_stru *mem = (struct memory_stru *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		printf("%s, %s, %d\n", __FILE__,__FUNCTION__,__LINE__);
		//exit(1);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

struct memory_stru response(http_t *http){
	struct memory_stru result;
	result.memory = malloc(1);
	result.size = 0;
	
	curl_easy_setopt(http->curl, CURLOPT_HTTPHEADER, http->headers);
	curl_easy_setopt(http->curl, CURLOPT_HEADER, 0);//设置非0则，输出包含头信息
	curl_easy_setopt(http->curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(http->curl, CURLOPT_CONNECTTIMEOUT, http->timeout);
	curl_easy_setopt(http->curl, CURLOPT_TIMEOUT, http->timeout);
	curl_easy_setopt(http->curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(http->curl, CURLOPT_WRITEDATA, (void *)&result);
	curl_easy_setopt(http->curl, CURLOPT_FOLLOWLOCATION, http->location);
	
	if(http->headers == NULL)
	{
		add_header(http, "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2922.1 Safari/537.36");
	}
	
	CURLcode res;
	res = curl_easy_perform(http->curl);
	
	long retcode = 0;
	res = curl_easy_getinfo(http->curl, CURLINFO_RESPONSE_CODE , &retcode);

	printf("%s, %s, %d, retcode:%d\n", __FILE__,__FUNCTION__,__LINE__, retcode);
	
	if(retcode == 301 || retcode == 302) 
	{
		res = curl_easy_getinfo(http->curl, CURLINFO_REDIRECT_URL , &http->redirect_url);
	}
	
	curl_easy_cleanup(http->curl);
	
	if(http->headers != NULL) 
		curl_slist_free_all(http->headers);
	
	return result;
}