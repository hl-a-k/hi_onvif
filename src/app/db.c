#include "db.h"
#include "uri_encode.h"

static int g_ipcamera_id;


/******************************************************************************
* Name:     GetIPFromRtsp 
*
* Desc:   读取设备的IP地址
* Param:  
    char *ip,   保存的ip地址
    char *rtsp,  
* Return:     TRUE:  操作成功，FALSE:操作失败 
* Global:      
* Note:  sample  rtsp://192.168.15.240:554/Streaming/Channels/2?transportmode=unicast 
 ******************************************************************************/
int __IPCameraGetIPFromRtsp(char *ip, char *rtsp)
{
    char *p;
    char *p1;
    int len=0;

    p = strstr(rtsp,"rtsp://");
    if(p)
    {
        p = p+strlen("rtsp://");
        p1 = strchr(p,'/');
        if(p1==NULL)
            return -1;
        len = p1-p;
        strncpy(ip,p, len);
        return 0;
    }
    else
        return -1;
}


/******************************************************************************
* Name: 	GetIPFromWebserver 
*
* Desc:   读取设备的IP地址，保存到链表中	 
* Param:  
    char *ip,   链表节点的ip地址
    char *XAddrs, webserve地址
* Return: 	TRUE:  添加成功，FALSE:添加失败 
* Global: 	 
* Note:  sample  http://192.168.0.243/onvif/device_service
 ******************************************************************************/
static int GetIPFromWebserver(char *ip, char *XAddrs)
{
    char *p;
    char *p1;
    int len=0;

    p = strstr(XAddrs,"http://");
    if(p)
    {
        p = p+strlen("http://");
        p1 = strchr(p,'/');
        if(p1==NULL)
            return 0;
        len = p1-p;
        strncpy(ip,p, len);
        return 1;
    }
    else
        return 0;
}


/******************************************************************************
* Name:     TripIPPort 
*
* Desc: 去除ip地址中的端口号
* Param:  
* Return:     TRUE:  添加成功，FALSE:添加失败 
* Global:      
* Note: 霍尼韦尔的摄像头获取到的IP中携带有端口 
        sample  192.168.0.243:2000
 ******************************************************************************/
int __IPCameraTripIPPort(char *dstip, char *srcip)
{
    char *p = NULL;
    int len = 0;

    if(srcip && dstip)
    {
         p = strchr(srcip,':');
         if(NULL == p)
         {
            strncpy(dstip, srcip, strlen(srcip));
            return -1;
         }
         else
         {
            len = p - srcip;
            strncpy(dstip, srcip, len);
            dstip[len] = '\0';
            return 0;
         }
    }
    return -1;
}


static int get_ipcamera_id_callback(void *NotUsed, int argc, char **argv, char **azColName){

    int id = atoi(argv[0]);
    if(id != g_ipcamera_id) {
        return 1;
    } else {
        g_ipcamera_id++;
    }
    return 0;
}

int get_ipcamera_id(sqlite3 *db)
{
    char *zErrMsg = 0;
    int rc;
    g_ipcamera_id = 0;
    rc = sqlite3_exec(db, "select id from ipcamera order by id", get_ipcamera_id_callback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    return g_ipcamera_id;
}

int add_device(sqlite3 *db,char *name, char *hardware, char *uuid, char *webserver, int *count) {
    char ipAddr[32]; 
    memset(ipAddr, 0, sizeof(ipAddr));
    GetIPFromWebserver(ipAddr, webserver);
    __IPCameraTripIPPort(ipAddr, ipAddr);

    uri_decode(name, strlen(name), name);

    char *query = "select 1 from ipcamera where ip = ?";
    char *add = "insert into ipcamera (id,name, hardware, uuid, webserver, ip, authorization, types, show_on_dashboard, location, visual_doorbell, support, record, enable, flag, type) values (?, ?, ?, ?, ?, ?, 0, 0, 0, '', 0, 0, 0, 0, 0, 0)";
    sqlite3_stmt *stmt;
    int err, cnt = 0;
    err = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (err != SQLITE_OK) {
        printf("prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, ipAddr, strlen(ipAddr) ,NULL);
    err = sqlite3_step(stmt);

    if (err == SQLITE_ROW) {
        cnt = sqlite3_column_count(stmt);
    } 
    sqlite3_finalize(stmt);

    if(cnt == 0) {
        err = sqlite3_prepare_v2(db, add, -1, &stmt, NULL);
        if (err != SQLITE_OK) {
            printf("prepare failed: %s\n", sqlite3_errmsg(db));
            return 1;
        }
        int seq = 1;
        sqlite3_bind_int(stmt, seq++, get_ipcamera_id(db));
        sqlite3_bind_text(stmt, seq++, name, strlen(name) ,NULL);
        sqlite3_bind_text(stmt, seq++, hardware, strlen(hardware) ,NULL);
        sqlite3_bind_text(stmt, seq++, uuid, strlen(uuid) ,NULL);
        sqlite3_bind_text(stmt, seq++, webserver, strlen(webserver) ,NULL);
        sqlite3_bind_text(stmt, seq++, ipAddr, strlen(ipAddr) ,NULL);
        sqlite3_step(stmt);
        (*count)++;
        sqlite3_finalize(stmt);
    }

    return 0;
}

int get_webserver(sqlite3 *db, int id, char** webserver) {

    char *query = "select webserver from ipcamera where id = ?";
    sqlite3_stmt *stmt;
    int err, cnt = 0;
    err = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (err != SQLITE_OK) {
        printf("prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_int(stmt, 1, id);
    err = sqlite3_step(stmt);
    if (err == SQLITE_ROW) {
       *webserver = strdup(sqlite3_column_text(stmt, 0));
    } 
    sqlite3_finalize(stmt);
    return 0;

}


int get_ipcamera_info(sqlite3 *db, int id, char** webserver, char** name, char** username, char** password) {

    char *query = "select webserver, name, username, password from ipcamera where id = ?";
    sqlite3_stmt *stmt;
    int err, cnt = 0;
    err = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (err != SQLITE_OK) {
        printf("prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_int(stmt, 1, id);
    err = sqlite3_step(stmt);
    if (err == SQLITE_ROW) {
       *webserver = strdup(sqlite3_column_text(stmt, 0));
       *name = strdup(sqlite3_column_text(stmt, 1));
       *username = strdup(sqlite3_column_text(stmt, 2));
       *password = strdup(sqlite3_column_text(stmt, 3));
    } 
    sqlite3_finalize(stmt);
    return 0;

}

int add_profile(sqlite3 *db, int d_id, char * name, char * token, char* videoEncoderConfigurationtoken,
int encoding, int h264Profile, int width, int height, int frameRateLimit, int bitrateLimit) {
    char *add = "insert into profile (d_id, name, token, videoEncoderConfigurationtoken, encoding, h264Profile, width, height, frameRateLimit, bitrateLimit) values (?,?,?,?,?,?,?,?,?,?)";
    sqlite3_stmt *stmt;
    int err = sqlite3_prepare_v2(db, add, -1, &stmt, NULL);
    if (err != SQLITE_OK) {
        printf("prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    int seq = 1;
    sqlite3_bind_int(stmt, seq++, d_id);
    sqlite3_bind_text(stmt, seq++, name, strlen(name) ,NULL);
    sqlite3_bind_text(stmt, seq++, token, strlen(token) ,NULL);
    sqlite3_bind_text(stmt, seq++, videoEncoderConfigurationtoken, strlen(videoEncoderConfigurationtoken) ,NULL);
    sqlite3_bind_int(stmt, seq++, encoding);
    sqlite3_bind_int(stmt, seq++, h264Profile);
    sqlite3_bind_int(stmt, seq++, width);
    sqlite3_bind_int(stmt, seq++, height);
    sqlite3_bind_int(stmt, seq++, frameRateLimit);
    sqlite3_bind_int(stmt, seq++, bitrateLimit);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int del_profile_by_d_id(sqlite3 *db, int d_id) {
     char *add = "delete from  profile where d_id = ?";
    sqlite3_stmt *stmt;
    int err = sqlite3_prepare_v2(db, add, -1, &stmt, NULL);
    if (err != SQLITE_OK) {
        printf("prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }
 
    sqlite3_bind_int(stmt, 1, d_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}


int get_profile(sqlite3 *db, int d_id, char** profileToken, int *profile_id, int *width, int *height) {
    char *query = "select token, id, width, height from profile where d_id = ? and encoding = 2 order by width * height limit 1";
    sqlite3_stmt *stmt;
    int err, rc = EFINDPROFILE;
    err = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

    if (err != SQLITE_OK) {
        printf("prepare failed: %s\n", sqlite3_errmsg(db));
        return AUTHFAIL;
    }

    sqlite3_bind_int(stmt, 1, d_id);
    sqlite3_bind_int(stmt, 2, MAXWIDTH);
    sqlite3_bind_int(stmt, 3, MAXHEIGHT);
    err = sqlite3_step(stmt);
    if (err == SQLITE_ROW) {
       *profileToken = strdup(sqlite3_column_text(stmt, 0));
       *profile_id = sqlite3_column_int(stmt, 1);
       *width = sqlite3_column_int(stmt, 2);
       *height = sqlite3_column_int(stmt, 3);
       if(*width > MAXWIDTH || *height > MAXHEIGHT) {
        rc = AUTHFAILRESOUTION;
       } else {
         rc = AUTHSUCCESS;
       }
    } else {
        rc = AUTHFAILH264;
    }
    sqlite3_finalize(stmt);
    return rc;
}

int set_device_rtsp(sqlite3 *db, int id, char *name, char *username, char *password, char *mediauri,  char * rtsp, int activeProfile, int width, int height) {
    char *query = "update ipcamera set name = ?, username = ?, password = ?, mediauri = ?, rtsp = ?,  activeProfile = ?, authorization = 1, support = 1,  width = ?, height = ? where id = ?";
    sqlite3_stmt *stmt;
    int err, cnt = 0;
    err = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (err != SQLITE_OK) {
        printf("prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    int seq = 1;
    sqlite3_bind_text(stmt, seq++, name, strlen(name) ,NULL);
    sqlite3_bind_text(stmt, seq++, username, strlen(username) ,NULL);
    sqlite3_bind_text(stmt, seq++, password, strlen(password) ,NULL);
    sqlite3_bind_text(stmt, seq++, mediauri, strlen(mediauri) ,NULL);
    sqlite3_bind_text(stmt, seq++, rtsp, strlen(rtsp) ,NULL);
    sqlite3_bind_int(stmt, seq++, activeProfile);
    sqlite3_bind_int(stmt, seq++, width);
    sqlite3_bind_int(stmt, seq++, height);
    sqlite3_bind_int(stmt, seq++, id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int add_device_manually(sqlite3 *db, char *name, char *username, char *password, char * rtsp) {
    char ip[50] = {0};
    __IPCameraGetIPFromRtsp(ip, rtsp);
    __IPCameraTripIPPort(ip, ip);

    char *add = "insert into ipcamera (id, name, username, password, rtsp, ip, authorization, types, activeProfile, show_on_dashboard, location, visual_doorbell, support, width, height, record, enable, flag, type) values (?, ?, ?, ?, ?, ?, 1, 1, 0, 0, '', 0, 1, 1280, 720, 0, 0, 0, 0)";
    sqlite3_stmt *stmt;
    int err = sqlite3_prepare_v2(db, add, -1, &stmt, NULL);
    if (err != SQLITE_OK) {
        printf("prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    int seq = 1;
    sqlite3_bind_int(stmt, seq++, get_ipcamera_id(db));
    sqlite3_bind_text(stmt, seq++, name, strlen(name) ,NULL);
    sqlite3_bind_text(stmt, seq++, username, strlen(username) ,NULL);
    sqlite3_bind_text(stmt, seq++, password, strlen(password) ,NULL);
    sqlite3_bind_text(stmt, seq++, rtsp, strlen(rtsp) ,NULL);
    sqlite3_bind_text(stmt, seq++, ip, strlen(ip) ,NULL);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int del_parts(sqlite3 *db, char * table ,char *ids) {
    char *sql = malloc(strlen(table) + strlen(ids) + 10);
    sprintf(sql, table, ids);
    char *err_msg = 0;
    int rc = sqlite3_exec(db,sql, 0, 0, &err_msg);
    free(sql);
    
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        sqlite3_close(db);  
        return 1;
    } 
    return 0;

}

int del_some(sqlite3 *db, char *ids) {
    char *delProfile = "delete from profile where d_id in (%s)";
    char *delIpcamera = "delete from ipcamera where id in (%s)";
    if(del_parts(db, delProfile, ids) != 0) {
        return 1;
    }

    if(del_parts(db, delIpcamera, ids) != 0) {
        return 1;
    }
   
    return 0;
}
int del_all(sqlite3 *db) {
    char *err_msg = 0;
    int rc = sqlite3_exec(db, "delete from profile", 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return 1;
    } 

    rc = sqlite3_exec(db, "delete from ipcamera", 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return 1;
    } 

    return 0;

}