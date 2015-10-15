#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <pthread.h>
#include <curl/curl.h>

const char* _url = "http://172.171.51.154:8888/txt/1.txt";
const char* _filename = "2.txt";
FILE* _file = NULL;
#define MAXBUFSIZE 1024
char* cache;

size_t write_file(char* buff, size_t size, size_t nmemb, void* cb_data)
{
	FILE* _file = (FILE*)cb_data;
	_file = fopen(_filename, "w+");
	fwrite(buff, size, nmemb, _file);
	fclose(_file);
}

size_t write_mem(char* buff, size_t size, size_t nmemb, void* cb_data)
{
	cache = (char*)malloc(MAXBUFSIZE);
	strncpy(cache, buff, MAXBUFSIZE);
	printf("cache %s\n", cache);	
}

int main(int argc, char** argv)
{
	CURL* curl = curl_easy_init();
	struct curl_slist*  _list = NULL;
	if (curl)
	{
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_URL, _url);
		_list = curl_slist_append(_list, "Accept:123");
		_list = curl_slist_append(_list, "yfcheng: yes");
		_list = curl_slist_append(_list, "key:value");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, _list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_mem);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, cache);
		res = curl_easy_perform(curl);
		if (CURLE_OK == res)
		{
			char* ct;
			res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
			if ((CURLE_OK == res) && ct)
			{
				printf("content-type: %s\n", ct);
			}
			long code;
			res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
			if ((CURLE_OK == res) && code)
			{
				printf("code %d\n", code);
			}
		}
		curl_easy_cleanup(curl);
		curl_slist_free_all(_list);
	}
	return 0;
}
