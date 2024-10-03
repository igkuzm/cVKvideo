#config to create cYandexDisk lib with automake

SUBDIRS = cVK
AUTOMAKE_OPTIONS = subdir-objects

CURL_LINK =

if WINDOWS
if X86_64
		CURL_LINKS += cVK/curl/windows/win64/lib/libcurl.dll.a
		INCLUDES += cVK/curl/windows/win64/include
endif
if I386
		CURL_LINKS += cVK/curl/windows/win32/lib/libcurl.dll.a
		INCLUDES += cVK/curl/windows/win32/include
endif
else
	CURL_LINK += -lcurl
endif


bin_PROGRAMS = vkvideo
cVKvideo_SOURCES = vkvideo.c \
									 cVKvideo.c

cVKvideo_CFLAGS = 
cVKvideo_LDFLAGS = $(CURL_LINK)
cVKvideo_LDADD = cVK/.libs/libcVK.a -lcurl
