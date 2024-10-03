/**
 * File              : vkvideo.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 03.10.2024
 * Last Modified Date: 04.10.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include "ini.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "cVK/cVK.h"
#include "config.h"
#include "strtok_foreach.h"
#include "fm.h"
#include <curl/curl.h>
#include "cVK/cJSON.h"
#include "getstr.h"
#include "cVKvideo.h"
#include "array.h"

char buffer[BUFSIZ];

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

struct video_search_t {
	array_t *videos;
	int iterator;
};

static int video_search_cb(
		void *data, cVKvideo_t *video, const char *error)
{
	struct video_search_t *t = data;
	array_append(t->videos, cVKvideo_t*, video, 
			perror("array_append"); return 1);
	
	printf("%d: ", t->iterator++);
	printf("%s\n", video->title);

	return 0;
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

		char *s = getstr(buffer, BUFSIZ);
		if (strcmp(s, "q") == 0 || strcmp(s, "Q") == 0)
			break;

		array_t *videos = 
			array_new(cVKvideo_t*, perror("array_new"); return 1);
		struct video_search_t t = 
			{videos, 1};

		c_vk_video_search(
				token, 
				s, 
				&t, 
				video_search_cb);
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
