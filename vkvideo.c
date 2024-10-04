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
#include <string.h>
#include "cVK/cJSON.h"
#include "getstr.h"
#include "cVKvideo.h"
#include "array.h"
#include "str.h"

#define DEFAULT_DOWNLOAD "yt-dlp"
#define DEFAULT_PLAYER "mplayer"

char buffer[BUFSIZ];

static void create_default_config(const char *path)
{
	FILE *fp = fopen(path, "w");
	if(!fp)
		return;
	fputs("#default config\n", fp);
	fputs("[cVKvideo]\n", fp);
	fputs("DOWLOAD=", fp);
	fputs(DEFAULT_DOWNLOAD, fp);
	fputs("\n", fp);
	fputs("PLAYER=", fp);
	fputs(DEFAULT_PLAYER, fp);
	fputs("\n", fp);
}

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
	char config_old[PATH_MAX] = {0};
	sprintf(config,
			"%s/.config/cVKvideo", home);
	
	strtok_foreach(config, "/", dir)
		newdir(config, 0744);	
	
	strcat(config, "/config");
	strcat(config_old, config);
	strcat(config_old, ".old");

	if (fcopy(config, config_old)){
		perror("file copy");
		return token;
	}

	if (ini_set(config_old, config, 
			"[cVK]", "TOKEN", token))
		perror("write ini file");
	
	return token;
}

static void print_video_in_list(cVKvideo_t *v, int idx)
{
	int i;
	printf("%d: ", idx);
	if (idx < 10)
		printf(" ");
	printf("%.40s", v->title);
	
	//add spaces
	int len = 0;
	if (v->title)
		len = strlen(v->title);
	if (len > 40)
		len = 40;
	for (i = len; i < 41; ++i)
		printf(" ");	

	if (v->duration){
		int hours = v->duration / 3600;
		int minuts = v->duration % 3600;
		int min = minuts / 60;
		int sec = minuts % 60;
		printf("[%d:%d:%d]", hours, min, sec);
	}
	printf("\n");
}

static void print_video_dscription(cVKvideo_t *v)
{
	printf("%s\n", v->title);
	if (v->duration){
		int hours = v->duration / 3600;
		int minuts = v->duration % 3600;
		int min = minuts / 60;
		int sec = minuts % 60;
		printf("[%d:%d:%d]\n", hours, min, sec);
	}
	printf("\n");
	if (v->description)
		printf("%s\n", v->description);
}

static int video_search_cb(
		void *data, cVKvideo_t *video, const char *error)
{
	if (error){
		printf("ERROR: %s\n",error);
		return 0;
	}
	if (video){
		array_t *videos = data;
		array_append(videos, cVKvideo_t*, video, 
				perror("array_append"); return 1);
	}

	return 0;
}

static int main_loop(
		const char *token, const char *config, char *arg)
{
	while (1) {
video_search:;
		char *s;
		if (arg){
			s = arg;
			printf("Search video: %s\n", s);	
		} else {
			printf( "\n");
			printf("_____________________________\n");
			printf("q - Quit\n");
			printf("_____________________________\n");
			printf("Search video: ");	
			
			s = getstr(buffer, BUFSIZ);
			if (strcmp(s, "q") == 0 || strcmp(s, "Q") == 0)
				break;
		}

		array_t *videos = 
			array_new(cVKvideo_t*, perror("array_new"); return 1);

		c_vk_video_search(
				token, 
				s, 
				videos, 
				video_search_cb);

video_list:;
		free(s);
		arg = NULL;
		int idx = 0;
		array_for_each(videos, cVKvideo_t*, video){
			print_video_in_list(video, ++idx);
		}

		// get video info
		while (1) {
			printf( "\n");
			printf("_____________________________\n");
			printf("q - Quit\n");
			printf("b - back to search\n");
			//printf("space/enter - next list\n");
			printf("_____________________________\n");
			printf("Select video number: ");	

			char *s = getstr(buffer, BUFSIZ);
			if (strcmp(s, "q") == 0 || strcmp(s, "Q") == 0)
				goto exit_main_loop;
			if (strcmp(s, "b") == 0 || strcmp(s, "B") == 0)
				goto video_search;
			if (strcmp(s, "") == 0 || strcmp(s, " ") == 0)
				goto video_list;

			int idx = atoi(s);
			if (idx > 0 && idx <= NUMBER_OF_VIDEOS){
				cVKvideo_t *video =
					((cVKvideo_t**)(videos->data))[idx-1];
				if (video){
video_description:
					while (1) {
						/*printf("%s\n", video->player); //log*/
						print_video_dscription(video);
						printf( "\n");
						printf("_____________________________\n");
						printf("q - Quit\n");
						printf("b - back to video list\n");
						printf("d - download video\n");
						printf("p - play video\n");
						printf("_____________________________\n");
						printf("Command: ");	
					
						char *s = getstr(buffer, BUFSIZ);
						if (strcmp(s, "q") == 0 || strcmp(s, "Q") == 0)
							goto exit_main_loop;
						if (strcmp(s, "b") == 0 || strcmp(s, "B") == 0)
							goto video_list;
						if (strcmp(s, "") == 0 || strcmp(s, " ") == 0)
							goto video_description;
						if (strcmp(s, "p") == 0 || strcmp(s, "P") == 0)
						{
							if (!video->player){
								printf("ERROR: video has no url\n");
								break;
							}
							char *cmd = ini_get(config,
									"cVKvideo", "PLAYER");
							if (!cmd)
								cmd = DEFAULT_PLAYER;
							sprintf(buffer, "%s '%s'", cmd, video->player);	
							printf("%s\n", buffer);
							system(buffer);
							printf("press any key\n");
							getchar();
						}
						if (strcmp(s, "d") == 0 || strcmp(s, "D") == 0)
						{
							if (!video->player){
								printf("ERROR: video has no url\n");
								break;
							}
							char *cmd = ini_get(config,
									"cVKvideo", "DOWLOAD");
							if (!cmd)
								cmd = DEFAULT_DOWNLOAD;
							sprintf(buffer, "%s '%s'", cmd, video->player);	
							printf("%s\n", buffer);
							system(buffer);
							printf("press any key\n");
							getchar();
						}
					}
				}
			}
		}
		{
			// free array
			array_for_each(videos, cVKvideo_t*, video)
				c_vk_video_free(video);
			array_free(videos);	
		}
	}

	exit_main_loop:;
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

	if (!fexists(config)){
		// create default config
		create_default_config(config);
	}

	char *token = 
		ini_get(config, "cVK", "TOKEN");
	if (!token)
		token = login();

	if(!token){
		printf("Err: can't login\n");
		return 1;
	}

	char *arg = NULL;
	if (argc > 1){
		struct str s;
		str_init(&s);
		int i;
		for (i = 1; i < argc; ++i) 
			str_appendf(&s, "%s ", argv[i]);
		
		arg = s.str;
	}

	main_loop(token, config, arg);

	free(token);
	return 0;
}
