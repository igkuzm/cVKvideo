#include "cVKvideo.h"
#include "cVK/cJSON.h"
#include "cVK/cVK.h"
#include "str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QUERY_BUF 256
#define STR(...)\
	({char _s[BUFSIZ];snprintf(_s,BUFSIZ,__VA_ARGS__);_s[BUFSIZ-1]=0;_s;})

#define GET_STR(obj, name, var)\
	({cJSON *item = cJSON_GetObjectItem(obj, name);\
	 if(item && item->valuestring)\
		var = strdup(item->valuestring);})

#define GET_INT(obj, name, var)\
	({cJSON *item = cJSON_GetObjectItem(obj, name);\
	 if(item) var = item->valueint;})


static cVKvideo_t *_c_vk_video_new_from_json(cJSON *o)
{
	cVKvideo_t *v = 
		(cVKvideo_t *)malloc(sizeof(cVKvideo_t));
	if (!v)
		return NULL;
	memset(v, 0, sizeof(cVKvideo_t));

	void *ptr = NULL;

	GET_STR(o, "privacy_view", v->privacy_view);
	GET_STR(o, "privacy_comment", v->privacy_comment);
	GET_INT(o, "can_comment", v->can_comment);
	GET_INT(o, "can_like", v->can_like);
	GET_INT(o, "can_repost", v->can_repost);
	GET_INT(o, "can_subscribe", v->can_subscribe);
	GET_INT(o, "can_add", v->can_add);
	GET_INT(o, "can_add_to_faves", v->can_add_to_faves);
	cJSON *likes = 
			cJSON_GetObjectItem(o, "likes");	
	if (cJSON_IsObject(likes)){
		GET_INT(likes, "user_likes", v->likes.user_likes);
		GET_INT(likes, "count", v->likes.count);
	}
	GET_INT(o, "adding_date", v->adding_date);
	GET_INT(o, "comments", v->comments);
	GET_INT(o, "date", v->date);
	GET_STR(o, "description", v->description);
	GET_INT(o, "duration", v->duration);
	cJSON *images = 
			cJSON_GetObjectItem(o, "images");	
	if (cJSON_IsArray(images)){
		int n = 0;
		int size = sizeof(struct {char *u;int w;int h;});
		ptr = malloc(size);
		if (ptr){
			v->images = ptr;
			cJSON *image;
			cJSON_ArrayForEach(image, images){
				if (image){
					GET_STR(image, "url", v->images[n].url);
					GET_INT(image, "width", v->images[n].width);
					GET_INT(image, "height", v->images[n].height);
					// realloc
					ptr = realloc(v->images, size * ++n);
					if (!ptr)
						break;
					v->images = ptr;
				}
			}
		}
		v->nimages = n;
	}
	GET_INT(o, "width", v->width);
	GET_INT(o, "height", v->height);
	GET_INT(o, "id", v->id);
	GET_INT(o, "owner_id", v->owner_id);
	GET_STR(o, "ov_id", v->ov_id);
	GET_STR(o, "title", v->title);
	GET_INT(o, "is_favorite", v->is_favorite);
	GET_STR(o, "player", v->player);
	GET_INT(o, "added", v->added);
	GET_INT(o, "repeat", v->repeat);
	GET_STR(o, "type", v->type);
	GET_INT(o, "views", v->views);
	cJSON *reposts = 
			cJSON_GetObjectItem(o, "reposts");	
	if (cJSON_IsObject(reposts)){
		GET_INT(reposts, "count", v->reposts.count);
		GET_INT(reposts, "user_reposted", v->reposts.user_reposted);
	}
	GET_STR(o, "track_code", v->track_code);

	return v;
};

cVKvideo_t *c_vk_video_new_from_json(const char *json)
{
	cJSON *o = cJSON_Parse(json);
	if (!cJSON_IsObject(o))
		return NULL;

	return _c_vk_video_new_from_json(o);
}

#define FREE(v) ({if(v) free(v); v=NULL;})
void c_vk_video_free(cVKvideo_t *v){
	if (v){
		FREE(v->privacy_view);
		FREE(v->privacy_comment);
		FREE(v->description);
		FREE(v->player);
		FREE(v->type);
		FREE(v->ov_id);
		FREE(v->title);
		FREE(v->track_code);
		if (v->images){
			int i;
			for (i = 0; i < v->nimages; ++i) {
				FREE(v->images[i].url);
			}
			free(v->images);
			v->images = NULL;
		}
		free(v);
	}
}

static char *make_escaped(const char *str){
	char *s = malloc(QUERY_BUF*3 + 1);
	if (!s)
		return NULL;

	int i, l=0;
	for (i = 0; str[i] && i < QUERY_BUF; ++i) {
		if (str[i] == ' ' || str[i] == '\t')
		{
			s[l++] = '%';
			s[l++] = '2';
			s[l++] = '0';
		} else {
			s[l++] = str[i];
		}
	}
	s[l] = 0;
	return s;
}

struct search_video_t{
	void *userdata;
	int (*callback)(
		void *userdata, cVKvideo_t *video, const char *err);
};

static void search_video_cb(
		void *d, const char *response, const char *error)
{
	/*printf("%s\n", response); //log*/
	
	struct search_video_t *t = (struct search_video_t *)d;  
	if (error){
		if(t->callback)
			t->callback(t->userdata, NULL, error);
		return;
	}
	cJSON *json = cJSON_Parse(response);
	if (!cJSON_IsObject(json)){
		if(t->callback)
			t->callback(t->userdata, NULL, 
					STR("can't parse json: %s\n", response));
		return;
	}
	cJSON *items = 
		cJSON_GetObjectItem(json, "items");
	if (!cJSON_IsArray(items)){
		if(t->callback)
			t->callback(t->userdata, NULL, 
					STR("no items, can't parse json: %s\n", response));
		return;
	}

	cJSON *item;
	cJSON_ArrayForEach(item, items){
		cVKvideo_t *video =
			_c_vk_video_new_from_json(item);
		if (video)
			if(t->callback)
				if(t->callback(t->userdata, video, NULL))
					break;
	}
	cJSON_free(json);
}

int c_vk_video_search(
		const char *token, 
		const char *query,
		void *userdata,
		int (*callback)(
			void *userdata, cVKvideo_t *video, const char *err))
{
	char q[QUERY_BUF] = {0};
	snprintf(q, QUERY_BUF -1, "q=%s", query);
	q[QUERY_BUF-1]=0;
	char *q_escaped = 
		make_escaped(q);
	if (!q_escaped){
		if(callback)
			callback(userdata, NULL, "can't parse query string");
		return 1;
	}

	char count[16];
	sprintf(count, "count=%d", NUMBER_OF_VIDEOS);

	struct search_video_t t = 
		{userdata, callback};

	int ret = c_vk_run_method(
			token, NULL,
			&t, search_video_cb,
			"video.search",	
			count,
			q_escaped,
			NULL);

	free(q_escaped);
	return ret;
}
