/**
 * File              : main.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 03.10.2024
 * Last Modified Date: 03.10.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include "ini.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "cVK/cVK.h"
#include "client_id.h"
#include "strtok_foreach.h"
#include "fm.h"
#include <curl/curl.h>
#include "cVK/cJSON.h"

static void login_cb(
		void *d, const char *t, int e, 
		const char *u, const char *error)
{
	if (error)
		printf("ERROR: %s\n", error);

	if (t){
		char *token = d;
		strcpy(token, t);
		printf("TOKEN: %s\n", token);
	}
}

static char *login(){
	printf("Need to login to Vkontakte\n");
	printf("open URL in browser and login\n");

	uint32_t access_rights = 
		AR_VIDEO
		| AR_OFFLINE;      

	char *url = 
		c_vk_auth_code_url(CLIENT_ID, access_rights);
	printf("URL: %s\n", url);

	char *token = malloc(256);
	if (!token)
		return NULL;
	*token = 0;

	c_vk_auth_token(
			CLIENT_ID, 
			CLIENT_SECRET, 
			token, 
			login_cb);

	if (!token)
		return NULL;

	char *home = getenv("HOME");
	char config[PATH_MAX] = {0};
	sprintf(config,
			"%s/.config/cVKvideo", home);
	
	strtok_foreach(config, "/", dir)
		newdir(config, 0744);	
	
	strcat(config, "/config");

	char str[BUFSIZ] = {0};
	strcat(str, "[cVK]\n");
	strcat(str, "TOKEN=");
	strcat(str, token);
	strcat(str, "\n");

	// write token to config
	FILE *fp = fopen(config, "w");
	if (!fp)
		return NULL;
	fwrite(str, strlen(str), 1, fp);
	fclose(fp);
	return token;
}

char *readString(){
	int i = 0;
	char *a = (char *) calloc(BUFSIZ, sizeof(char));
	if (!a) {
		fprintf(stderr, "ERROR. Cannot allocate memory\n");		
		return NULL;
	}										
	
	while (1) {
		scanf("%c", &a[i]);
		if (a[i] == '\n') {
			break;
		}
		else {
			i++;
		}
	}
	a[i] = '\0';
	return a;
}

void search_video_cb(
		void *d, const char *response, const char *error)
{
	if (error)
		perror(error);

	if (response){
		cJSON *json = cJSON_Parse(response);
		if (!json || !cJSON_IsObject(json)){
			printf("Err: can't parse json response\n");
			return;
		}
		cJSON *items = 
			cJSON_GetObjectItem(json, "items");
		if (!items)
			return;

		int i = 1;
		cJSON *item;
		cJSON_ArrayForEach(item, items){
			printf("%d: ", i++);
			cJSON *title = 
				cJSON_GetObjectItem(item, "title");
			if (title)
				printf("%s\n", title->valuestring);
			cJSON *description = 
				cJSON_GetObjectItem(item, "description");
			if (description)
				printf("[%s]\n", description->valuestring);
			cJSON *duration = 
				cJSON_GetObjectItem(item, "duration");
			if (duration)
				printf("%d min\n", description->valueint);
			printf("_______________________________\n");
		}
	}
}

void search_video(const char *token, const char *query)
{
	
	CURL *curl = curl_easy_init();
	if (!curl){
		printf("Err: can't init curl\n");
		return;
	}
	
	char q[BUFSIZ] = {0};
	sprintf(q, "q=%s", query);
	char *q_escaped = 
		curl_easy_escape(curl, q, 0);
	if (!q_escaped){
		printf("Err: can't parse query string\n");
		return;
	}

	char s[BUFSIZ];
	c_vk_run_method(
			token, NULL,
			s, search_video_cb,
			"video.search",	
			q_escaped,
			NULL);

	curl_free(q_escaped);
}

static int main_loop(const char *token)
{
	while (1) {
		char menuString[512];		
		sprintf(menuString, "%s\n", "");
		sprintf(menuString, "%s%s\n",menuString, "_____________________________");
		sprintf(menuString, "%s%s\n",menuString, "q - Quit");
		sprintf(menuString, "%s%s\n",menuString, "_____________________________");
		printf("%s",menuString);	

		printf("Search video:\n");	

		char *s = readString();
		if (strcmp(s, "q") == 0 || strcmp(s, "Q") == 0)
			break;


	}

	return 0;
}

int main(int argc, char *argv[])
{
	printf("cVK init\n");
	// try to open ini file
	char *home = getenv("HOME");
	char config[PATH_MAX] = {0};
	sprintf(config,
			"%s/.config/cVKvideo/config", home);
	
	char *token = 
		ini_get(config, "cVK", "TOKEN");
	if (!token)
		token = login();

	if(!token){
		printf("Err: can't login\n");
		return 1;
	}

	main_loop(token);

	free(token);
	return 0;
}
