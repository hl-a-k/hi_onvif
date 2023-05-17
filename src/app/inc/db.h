#ifndef DB_H
#define DB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "onvif-errno.h"

#define  MAXWIDTH           1920 //640
#define  MAXHEIGHT          1080 //480

int add_device(sqlite3 *db, char *name, char *hardware, char *uuid, char *webserver, int *count);
int get_webserver(sqlite3 *db, int id, char** webserver);
int get_ipcamera_info(sqlite3 *db, int id, char** webserver, char** name, char** username, char** password);
int add_profile(sqlite3 *db, int d_id, char * name, char * token, char* videoEncoderConfigurationtoken,int encoding, int h264Profile, int width, int height, int frameRateLimit, int bitrateLimit);
int del_profile_by_d_id(sqlite3 *db, int d_id);
int get_profile(sqlite3 *db, int d_id, char** profileToken, int *profile_id, int *width, int *height);
int set_device_rtsp(sqlite3 *db, int id,  char *name, char *username, char *password, char *mediauri,  char * rtsp, int activeProfile, int width, int height);
int add_device_manually(sqlite3 *db, char *name, char *username, char *password, char * rtsp);
int del_some(sqlite3 *db, char *ids);
int del_all(sqlite3 *db);
#endif