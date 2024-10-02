#config to create cYandexDisk lib with automake

CURL_LINK =

if WINDOWS
if X86_64
		CURL_LINKS += curl/windows/win64/lib/libcurl.dll.a
		INCLUDES += curl/windows/win64/include
endif
if I386
		CURL_LINKS += curl/windows/win32/lib/libcurl.dll.a
		INCLUDES += curl/windows/win32/include
endif
else
	CURL_LINK += -lcurl
endif

SUBDIRS = cVK
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cVKvideo
cVKvideo_SOURCES = \
									 main.c

cVKvideo_CFLAGS = 
cVKvideo_LDADD = \
								 $(CURL_LINK) \
								 cVK/.libs/libcVK.a
