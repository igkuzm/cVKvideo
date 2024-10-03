#ifndef C_VK_VIDEO_H
#define C_VK_VIDEO_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

typedef struct cVKvideo {
	char *privacy_view;	
	char *privacy_comment;
	bool can_comment;
	bool can_like;
	bool can_repost;
	bool can_subscribe;
	bool can_add;
	bool can_add_to_faves;
	struct{
		bool user_likes;
		int count;
	} likes;
	time_t adding_date;
	int comments;
	time_t date;
	char *description;
	int duration;
	struct {
		char *url;
		int width;
		int height;
	} *images;
	int nimages;
	int width;
	int height;
	int id;
	int owner_id;
	char *ov_id;
	char *title;
	bool is_favorite;
	char *player;
	int added;
	int repeat;
	char *type;
	int views;
	struct{
		int count;
		int user_reposted;
	} reposts;
	char *track_code;
} cVKvideo_t;

/* c_vk_video_search
 * search VK video with query string and callback allocated
 * cVKvideo_t for each - the caller of the function is
 * ownership of the data and is responsable for freeing it
 * To stop function execution return non-null in callback
 * Function return 0 on success and 1 on error */
int c_vk_video_search(
		const char *token, 
		const char *query,
		void *userdata,
		int (*callback)(
			void *userdata, cVKvideo_t *video, const char *err));


/* allocate cVKvideo_t from json string */
cVKvideo_t *c_vk_video_new_from_json(const char *json);

/* free cVKvideo_t */
void c_vk_video_free(cVKvideo_t *video);
#endif /* ifndef C_VK_VIDEO_H */
