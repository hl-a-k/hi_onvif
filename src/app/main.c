/******************************************************************************
* Name: 	 onvif app
*
* Desc:		  desc
* Param:
* Return:
* Global:
* Note: 
* Author:	 Omar.Zhou
* -------------------------------------
* Log: 	 2022/11/22, Create this funtion by Omar.Zhou
* ******************************************************************************/

#include <string.h>
#include <unistd.h>
#include <sqlite3.h>

#include "soapStub.h"
#include "OnvifBinding.nsmap"
#include "soapH.h"
#include "wsseapi.h"
#include "onvif-errno.h"
#include "db.h"

#define CONNECT_TIMEOUT 10
#define RECV_TIMEOUT 10

struct soap *new_soap() {
    //soap初始化，申请空间
    struct soap *soap = soap_new();
    if (soap == NULL) {
        fprintf(stderr, "func:%s,line:%d.malloc soap error.\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    soap_set_namespaces(soap, namespaces);
    soap_set_mode(soap, SOAP_C_UTFSTRING);
    soap->connect_timeout = CONNECT_TIMEOUT;
    soap->recv_timeout = RECV_TIMEOUT;
    return soap;
}

void del_soap(struct soap *soap) {
    if(soap != NULL) {
        //清除soap
        soap_end(soap);
        soap_free(soap);
    }
    
}

#define MAX_DEVICE 20


static int GetHardwareFromScope(char *hardware, char *scope)
{
    char * p;
    char * p1;
    int len=0;
    
    p = strstr(scope,"/hardware/");
    if(p)
    {
        p = p+strlen("/hardware/");
        p1 = strchr(p,' ');
        if(p1==NULL)        
            len = strlen(p);        
        else        
            len = p1-p;         
        strncpy(hardware, p, len);
        return 0;
    }
    else
        return 1;
}

static int GetNameFromScope(char *name, char *scope)
{
    char * p;
    char * p1;
    int len=0;

    p = strstr(scope,"/name/");
    if(p)
    {
        p = p+strlen("/name/");        
        p1 = strstr(p,"onvif://");
        if(p1==NULL)        
            len = strlen(p);        
        else        
            len = p1-p;   
                    
        strncpy(name, p, len);
        return 0;
    }
    else
        return 1;
}

int send_probe(struct soap *soap, sqlite3 *db, char *type, char *item, int *count) {
    struct __wsdd__ProbeMatches resp;
    char hardware[256] = {0};
    char name[256] = {0};

    struct wsdd__ProbeType req;   //用于发送消息描述   
    struct wsdd__ProbeType wsdd__Probe;
    
    //struct wsdd__ProbeMatchesType resp; //请求消息的回应
    struct wsdd__ScopesType sScope; //描述查找哪类的Web服务
    struct SOAP_ENV__Header header; //soap消息头描述
   
    int result = 0;    //返回值        
	unsigned char macaddr[6];  
	char _HwId[1024];  
	unsigned int Flagrand;
   

	// Create SessionID randomly  
	srand((int)time(0));  
	Flagrand = rand()%9000 + 8888;   
	macaddr[0] = 0x1;
	macaddr[1] = 0x2; 
	macaddr[2] = 0x3; 
	macaddr[3] = 0x4; 
	macaddr[4] = 0x5; 
	macaddr[5] = 0x6;  
	sprintf(_HwId,"urn:uuid:%ud68a-1dd2-11b2-a105-%02X%02X%02X%02X%02X%02X", Flagrand, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    /************初始化*************/


    soap_default_SOAP_ENV__Header(soap, &header);//将header设置为soap消息    头属性
    header.wsa__MessageID = _HwId;
    header.wsa__To     = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";
    header.wsa__Action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
    soap->header = &header; //设置soap头消息的ID
    //设置soap消息的请求服务属性
    soap_default_wsdd__ScopesType(soap, &sScope);
    sScope.__item = item;     
	//sScope.__item = "onvif://www.onvif.org";
    soap_default_wsdd__ProbeType(soap, &req);
    req.Scopes = &sScope;
    //req.Types = "tdn:NetworkVideoTransmitter";
    req.Types = type;
    //调用gSoap接口
    //soap_wsdd_Probe
    result  = soap_send___wsdd__Probe(soap, "soap.udp://239.255.255.250:3702", NULL, &req);
    printf("%s: %d, send probe request success!\n",__FUNCTION__, __LINE__);

    if ( result == -1) {
        printf ( "soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
        result = soap->error;
    } else {
        do {
            printf("%s: %d, begin receive probematch... \n",__FUNCTION__, __LINE__);
            printf("count=%d \n",count);
            //接收ProbeMatches,成功返回0,否则-1
            result = soap_recv___wsdd__ProbeMatches(soap, &resp);
            printf(" --soap_recv___wsdd__ProbeMatches() result=%d \n",result);
            if (result==-1) {
                break;
            } else {                     
                //读取服务端回应的Probematch消息
				if ( resp.wsdd__ProbeMatches){
                    memset(name, 0, sizeof(name));
                    GetHardwareFromScope(hardware,  resp.wsdd__ProbeMatches->ProbeMatch->Scopes->__item);
                    GetNameFromScope(name,  resp.wsdd__ProbeMatches->ProbeMatch->Scopes->__item);
                    add_device(db, name, hardware,  resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.Address, resp.wsdd__ProbeMatches->ProbeMatch->XAddrs, count);
				}
            }
        } while ( 1);
    }

    return result;

}

int discovery(struct soap *soap, sqlite3 *db) {   
    int count = 0;  //获得的设信息备个数 
    
    send_probe(soap, db, "tdn:NetworkVideoTransmitter", "onvif://www.onvif.org", &count);
    send_probe(soap, db, "", "", &count);
    
    printf("Find %d devices!\n", count);
    return 0;
}


int set_auth_info(struct soap *soap, const char *username, const char *password) {
    if (NULL == username) {
        printf("func:%s,line:%d.username is null.\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (NULL == password) {
        printf("func:%s,line:%d.password is nil.\n", __FUNCTION__, __LINE__);
        return -2;
    }

    int result = soap_wsse_add_UsernameTokenDigest(soap, NULL, username, password);

    return result;
}

int get_device_info(struct soap *soap, const char *username, const char *password, char *xAddr) {
    if (NULL == xAddr) {
        printf("func:%s,line:%d.dev addr is nil.\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (soap == NULL) {
        printf("func:%s,line:%d.malloc soap error.\n", __FUNCTION__, __LINE__);
        return -2;
    }

    struct _tds__GetDeviceInformation deviceInformation;
    struct _tds__GetDeviceInformationResponse deviceInformationResponse;

    set_auth_info(soap, username, password);

    int res = soap_call___tds__GetDeviceInformation(soap, xAddr, NULL, &deviceInformation, &deviceInformationResponse);

    if (NULL != soap) {
        printf("Manufacturer:%s\n", deviceInformationResponse.Manufacturer);
        printf("Model:%s\n", deviceInformationResponse.Model);
        printf("FirmwareVersion:%s\n", deviceInformationResponse.FirmwareVersion);
        printf("SerialNumber:%s\n", deviceInformationResponse.SerialNumber);
        printf("HardwareId:%s\n", deviceInformationResponse.HardwareId);
    }
    return res;
}

int get_capabilities(struct soap *soap, const char *username, const char *password, char *xAddr, char *mediaAddr) {
    struct _tds__GetCapabilities capabilities={0};
    struct _tds__GetCapabilitiesResponse capabilitiesResponse={0};
    enum tt__CapabilityCategory temp_category = tt__CapabilityCategory__Media;
    capabilities.__sizeCategory=1;
    capabilities.Category=&temp_category;

    set_auth_info(soap, username, password);
    int res = soap_call___tds__GetCapabilities(soap, xAddr, NULL, &capabilities, &capabilitiesResponse);
    if (soap->error) {
        printf("func:%s,line:%d.soap error: %d, %s, %s\n", __FUNCTION__, __LINE__, soap->error, *soap_faultcode(soap),
               *soap_faultstring(soap));
        return soap->error;
    }
    if (capabilitiesResponse.Capabilities == NULL) {
        printf("func:%s,line:%d.GetCapabilities  failed!  result=%d \n", __FUNCTION__, __LINE__, res);
    } else {
        printf("func:%s,line:%d.Media->XAddr=%s \n", __FUNCTION__, __LINE__,
               capabilitiesResponse.Capabilities->Media->XAddr);
        strcpy(mediaAddr, capabilitiesResponse.Capabilities->Media->XAddr);
    }
    return res;
}

int get_profiles(struct soap *soap, const char *username, const char *password, char *xAddr, int d_id, sqlite3 *db) {
    struct _trt__GetProfiles profiles;
    struct _trt__GetProfilesResponse profilesResponse;
    set_auth_info(soap, username, password);
    int res = soap_call___trt__GetProfiles(soap, xAddr, NULL, &profiles, &profilesResponse);
    if (res == -1)
        //NOTE: it may be regular if result isn't SOAP_OK.Because some attributes aren't supported by server.
        //any question email leoluopy@gmail.com
    {
        printf("func:%s,line:%d.soap error: %d, %s, %s\n", __FUNCTION__, __LINE__, soap->error, *soap_faultcode(soap),
               *soap_faultstring(soap));
        return soap->error;
    }

    if (profilesResponse.__sizeProfiles <= 0) {
        printf("func:%s,line:%d.Profiles Get Error\n", __FUNCTION__, __LINE__);
        return EGETPROFILES;
    }

    del_profile_by_d_id(db, d_id);

    for (int i = 0; i < profilesResponse.__sizeProfiles; i++) {
        if (profilesResponse.Profiles[i].token != NULL) {
            // printf("func:%s,line:%d.Profiles token:%s\n", __FUNCTION__, __LINE__, profilesResponse.Profiles->Name);

            //默认我们取第一个即可，可以优化使用字符串数组存储多个
            // if (i == 0) {
            //     strcpy(profileToken, profilesResponse.Profiles[i].token);
            // }

            if((NULL == profilesResponse.Profiles[i].Name) ||\
					(NULL == profilesResponse.Profiles[i].token) ||\
					(NULL == profilesResponse.Profiles[i].VideoEncoderConfiguration) ||\
					(NULL == profilesResponse.Profiles[i].VideoEncoderConfiguration->Name) ||\
					(NULL == profilesResponse.Profiles[i].VideoEncoderConfiguration->token) ||\
					(NULL == profilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution) ||\
					(NULL == profilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl) ||\
					(NULL == profilesResponse.Profiles[i].VideoEncoderConfiguration->H264))
				{
					printf(" ERROR! Profiles == NULL, %p,%p,%p,%p,%p,%p,%p,%p\n", profilesResponse.Profiles[i].Name,\
																				profilesResponse.Profiles[i].token,\
																				profilesResponse.Profiles[i].VideoEncoderConfiguration,\
																				profilesResponse.Profiles[i].VideoEncoderConfiguration->Name,\
																				profilesResponse.Profiles[i].VideoEncoderConfiguration->token,\
																				profilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution,\
																				profilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl,\
																				profilesResponse.Profiles[i].VideoEncoderConfiguration->H264);

				
		
					continue;
				}
				
               	printf(" Profiles[%d]->Name=%s \n", i,profilesResponse.Profiles[i].Name);
                printf(" Profiles[%d]->token=%s \n",i,profilesResponse.Profiles[i].token);
                printf(" Profiles[%d]->fixed=%s \n", i,profilesResponse.Profiles[i].fixed);
                printf(" Profiles[%d]->VideoEncoderConfiguration->Name=%s \n", i,profilesResponse.Profiles[i].VideoEncoderConfiguration->Name);  
                printf(" Profiles[%d]->VideoEncoderConfiguration->token=%s \n", i,profilesResponse.Profiles[i].VideoEncoderConfiguration->token);
                printf(" Profiles[%d]->VideoEncoderConfiguration->Encoding=%d \n", i,profilesResponse.Profiles[i].VideoEncoderConfiguration->Encoding);
                printf(" Profiles[%d]->VideoEncoderConfiguration->Resolution->Width=%d \n", i,profilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution->Width);
                printf(" Profiles[%d]->VideoEncoderConfiguration->Resolution->Height=%d \n", i,profilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution->Height);
                printf(" Profiles[%d]->VideoEncoderConfiguration->RateControl->FrameRateLimit=%d \n", i,profilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl->FrameRateLimit);
                printf(" Profiles[%d]->VideoEncoderConfiguration->RateControl->BitrateLimit=%d \n", i,profilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl->BitrateLimit);
                printf(" Profiles[%d]->VideoEncoderConfiguration->H264->H264Profile=%d \n", i,profilesResponse.Profiles[i].VideoEncoderConfiguration->H264->H264Profile);

                add_profile(db, d_id, profilesResponse.Profiles[i].Name, profilesResponse.Profiles[i].token, profilesResponse.Profiles[i].VideoEncoderConfiguration->token,
                profilesResponse.Profiles[i].VideoEncoderConfiguration->Encoding, profilesResponse.Profiles[i].VideoEncoderConfiguration->H264->H264Profile, profilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution->Width,
                profilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution->Height,
                profilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl->FrameRateLimit,
                profilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl->BitrateLimit
                );
                
        }
    }
    res = 0;
    return res;
}

int get_rtsp_uri(struct soap *soap, const char *username, const char *password, char *profileToken, char *xAddr, char **rtspUrl) {
    struct _trt__GetStreamUri streamUri;
    struct _trt__GetStreamUriResponse streamUriResponse;
    streamUri.StreamSetup = (struct tt__StreamSetup *) soap_malloc(soap, sizeof(struct tt__StreamSetup));
    streamUri.StreamSetup->Stream = 0;
    streamUri.StreamSetup->Transport = (struct tt__Transport *) soap_malloc(soap, sizeof(struct tt__Transport));
    streamUri.StreamSetup->Transport->Protocol = 0;
    streamUri.StreamSetup->Transport->Tunnel = 0;
    streamUri.StreamSetup->__size = 1;
    streamUri.StreamSetup->__any = NULL;
    streamUri.StreamSetup->__anyAttribute = NULL;
    streamUri.ProfileToken = profileToken;
    set_auth_info(soap, username, password);
    int res = soap_call___trt__GetStreamUri(soap, xAddr, NULL, &streamUri, &streamUriResponse);
    if (soap->error) {
        printf("func:%s,line:%d.soap error: %d, %s, %s\n", __FUNCTION__, __LINE__, soap->error, *soap_faultcode(soap),
               *soap_faultstring(soap));
        return soap->error;
    }
    printf("func:%s,line:%d.RTSP uri is :%s \n", __FUNCTION__, __LINE__, streamUriResponse.MediaUri->Uri);
    *rtspUrl = strdup(streamUriResponse.MediaUri->Uri);
    return 0;
}

int get_snapshot(struct soap *soap, const char *username, const char *password, char *profileToken, char *xAddr) {
    struct _trt__GetSnapshotUri snapshotUri;
    struct _trt__GetSnapshotUriResponse snapshotUriResponse;

    set_auth_info(soap, username, password);
    snapshotUri.ProfileToken = profileToken;
    int res = soap_call___trt__GetSnapshotUri(soap, xAddr, NULL, &snapshotUri, &snapshotUriResponse);
    if (soap->error) {
        printf("func:%s,line:%d.soap error: %d, %s, %s\n", __FUNCTION__, __LINE__, soap->error, *soap_faultcode(soap),
               *soap_faultstring(soap));
        return soap->error;
    }
    printf("func:%s,line:%d.Snapshot uri is :%s \n", __FUNCTION__, __LINE__,
           snapshotUriResponse.MediaUri->Uri);
    return res;
}

int ptz(struct soap *soap, const char *username, const char *password, int direction, float speed, char *profileToken,
        char *xAddr) {
    struct _tptz__ContinuousMove continuousMove = {0};
    struct _tptz__ContinuousMoveResponse continuousMoveResponse;
    struct _tptz__Stop tptzStop = {0};
    struct _tptz__StopResponse stopResponse;
    int res;

    set_auth_info(soap, username, password);
    continuousMove.ProfileToken = profileToken;
    continuousMove.Velocity = (struct tt__PTZSpeed *)soap_malloc(soap, sizeof(struct tt__PTZSpeed));
    continuousMove.Velocity->PanTilt = (struct tt__Vector2D *)soap_malloc(soap, sizeof(struct tt__Vector2D));
    memset(continuousMove.Velocity, 0,  sizeof(struct tt__PTZSpeed));
    memset(continuousMove.Velocity->PanTilt, 0,  sizeof(struct tt__Vector2D));

    switch (direction) {
        case 1:
            continuousMove.Velocity->PanTilt->x = 0;
            continuousMove.Velocity->PanTilt->y = speed;
            break;
        case 2:
            continuousMove.Velocity->PanTilt->x = 0;
            continuousMove.Velocity->PanTilt->y = -speed;
            break;
        case 3:
            continuousMove.Velocity->PanTilt->x = -speed;
            continuousMove.Velocity->PanTilt->y = 0;
            break;
        case 4:
            continuousMove.Velocity->PanTilt->x = speed;
            continuousMove.Velocity->PanTilt->y = 0;
            break;
        case 5:
            continuousMove.Velocity->PanTilt->x = -speed;
            continuousMove.Velocity->PanTilt->y = speed;
            break;
        case 6:
            continuousMove.Velocity->PanTilt->x = -speed;
            continuousMove.Velocity->PanTilt->y = -speed;
            break;
        case 7:
            continuousMove.Velocity->PanTilt->x = speed;
            continuousMove.Velocity->PanTilt->y = speed;
            break;
        case 8:
            continuousMove.Velocity->PanTilt->x = speed;
            continuousMove.Velocity->PanTilt->y = -speed;
            break;
        case 9:
            tptzStop.ProfileToken = profileToken;
            res = soap_call___tptz__Stop(soap, xAddr, NULL, &tptzStop, &stopResponse);
            if (soap->error) {
                printf("func:%s,line:%d.soap error: %d, %s, %s\n", __FUNCTION__, __LINE__, soap->error, *soap_faultcode(soap),
                       *soap_faultstring(soap));
                return soap->error;
            }
            return res;
        default:
            printf("func:%s,line:%d.Ptz direction unknown.\n", __FUNCTION__, __LINE__);
            return -1;
    }
    printf("---1\n\n xAddr:%s", xAddr);
    res = soap_call___tptz__ContinuousMove(soap, xAddr, NULL, &continuousMove, &continuousMoveResponse);
    if (soap->error) {
        printf("func:%s,line:%d.soap error: %d, %s, %s\n", __FUNCTION__, __LINE__, soap->error, *soap_faultcode(soap),
               *soap_faultstring(soap));
        return soap->error;
    }
    return res;
}

int handleSearch(int argc, char *argv[]) {
    struct soap *soap = NULL;
    sqlite3 *db = NULL;
    char *dbpath = argv[2];
    int rc;
    do{
        rc = sqlite3_open(dbpath, &db);
        if( rc ){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            break;
        }

        soap = new_soap();
        if(soap == NULL) {
            rc = ECREATESOAP;
            break;
        }
        discovery(soap, db);
        del_soap(soap);
        rc = 0;
    }while(0);

    sqlite3_close(db);
    return rc;
}


int handleAuth(int argc, char *argv[]) {
    if(argc<6)
    {
        printf(" --ERROR: hi_onvif: invalid option! \n");
        printf(" Usage: ./hi_onvif 1 id username password \n");
        return EINVAL;
    }

    int id = atoi(argv[3]);
    char *name = argv[4];
    char *username = argv[5];
    char *password = argv[6];

    struct soap *soap = NULL;
    sqlite3 *db = NULL;
    char *dbpath = argv[2];
    int rc;
    char* webserver = NULL;
    char* profileToken = NULL;
    char* rtspUrl = NULL;
    int profile_id;
    int width;
    int height;
    do{
        rc = sqlite3_open(dbpath, &db);
        if( rc ){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            rc = AUTHFAIL;
            break;
        }

        soap = new_soap();
        if(soap == NULL) {
            rc = AUTHFAIL;
            break;
        }

        get_webserver(db, id, &webserver);
        if(webserver == NULL) {
            rc = AUTHFAIL;
            break;
        }

        char mediaAddr[200] = {'\0'};
        rc = get_capabilities(soap, username, password, webserver, mediaAddr);
        if(rc != 0) {
            rc = AUTHFAIL;
            break;
        }
        rc = get_profiles(soap, username, password, mediaAddr, id, db);
        if(rc != 0) {
            rc = AUTHFAIL;
            break;
        }

        rc = get_profile(db, id, &profileToken, &profile_id, &width, &height);
        if(rc != 0) {
            break;
        }
        rc = get_rtsp_uri(soap, username, password, profileToken, mediaAddr, &rtspUrl);
        if(rc != 0) {
            rc = AUTHFAIL;
            break;
        }

        rc = set_device_rtsp(db, id, name, username, password, mediaAddr, rtspUrl, profile_id, width, height);
    }while(0);

    if(webserver != NULL) {
        free(webserver);
    }
    if(profileToken != NULL) {
        free(profileToken);
    }
    if(soap != NULL) {
        del_soap(soap);
    }
    sqlite3_close(db);
    return rc;
}

int handleDelAll(int argc, char *argv[]) {

    sqlite3 *db = NULL;
    char *dbpath = argv[2];
    int rc;
    do{
        rc = sqlite3_open(dbpath, &db);
        if( rc ){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            break;
        }
        rc = del_all(db);

    }while(0);

    sqlite3_close(db);
    return rc;
}

int handleDelSome(int argc, char *argv[]) {
    if(argc<3)
    {
        printf(" --ERROR: hi_onvif: invalid option! \n");
        printf(" Usage: ./hi_onvif 2 username password rtsp\n");
        return EINVAL;
    }

    char *ids = argv[3];
   
    sqlite3 *db = NULL;
    char *dbpath = argv[2];
    int rc;
    do{
        rc = sqlite3_open(dbpath, &db);
        if( rc ){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            break;
        }
        rc = del_some(db, ids);

    }while(0);

    sqlite3_close(db);
    return rc;
}


int handleManualAdd(int argc, char *argv[]) {
    if(argc<6)
    {
        printf(" --ERROR: hi_onvif: invalid option! \n");
        printf(" Usage: ./hi_onvif 2 name username password rtsp\n");
        return EINVAL;
    }

    char *name = argv[3];
    char *username = argv[4];
    char *password = argv[5];
    char *rtsp = argv[6];

    sqlite3 *db = NULL;
    char *dbpath = argv[2];
    int rc;
    do{
        rc = sqlite3_open(dbpath, &db);
        if( rc ){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            break;
        }
        rc = add_device_manually(db, name, username, password, rtsp);

    }while(0);

    sqlite3_close(db);
    return rc;
}


int handleReauth(int argc, char *argv[]) {
    if(argc<3)
    {
        printf(" --ERROR: hi_onvif: invalid option! \n");
        printf(" Usage: ./hi_onvif 5 db_path id \n");
        return EINVAL;
    }

    int id = atoi(argv[3]);
   

    struct soap *soap = NULL;
    sqlite3 *db = NULL;
    char *dbpath = argv[2];
    int rc;
    char* webserver = NULL;
    char* name = NULL;
    char *username = NULL;
    char* password = NULL;
    char* profileToken = NULL;
    char* rtspUrl = NULL;
    int profile_id;
    int width;
    int height;
    do{
        rc = sqlite3_open(dbpath, &db);
        if( rc ){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            rc = AUTHFAIL;
            break;
        }

        soap = new_soap();
        if(soap == NULL) {
            rc = AUTHFAIL;
            break;
        }

        get_ipcamera_info(db, id, &webserver, &name, &username, &password);
        if(webserver == NULL) {
            rc = AUTHFAIL;
            break;
        }

        char mediaAddr[200] = {'\0'};
        rc = get_capabilities(soap, username, password, webserver, mediaAddr);
        if(rc != 0) {
            rc = AUTHFAIL;
            break;
        }
        rc = get_profiles(soap, username, password, mediaAddr, id, db);
        if(rc != 0) {
            rc = AUTHFAIL;
            break;
        }

        rc = get_profile(db, id, &profileToken, &profile_id, &width, &height);
        if(rc != 0) {
            break;
        }
        rc = get_rtsp_uri(soap, username, password, profileToken, mediaAddr, &rtspUrl);
        if(rc != 0) {
            rc = AUTHFAIL;
            break;
        }

        rc = set_device_rtsp(db, id, name, username, password, mediaAddr, rtspUrl, profile_id, width, height);
    }while(0);

    if(webserver != NULL) {
        free(webserver);
    }
    if(name != NULL) {
        free(name);
    }
    if(username != NULL) {
        free(username);
    }
    if(password != NULL) {
        free(password);
    }
    if(profileToken != NULL) {
        free(profileToken);
    }
    if(soap != NULL) {
        del_soap(soap);
    }
    sqlite3_close(db);
    return rc;
}

int main(int argc, char *argv[]) {
    if(argc<3)
    {
        printf(" --ERROR: hi_onvif: invalid option! \n");
        printf(" Usage: ./hi_onvif operation db ... \n");
        return EINVAL;
    }

    int rc = EINVAL;
    int operation = atoi(argv[1]);

    if(operation == 0) { // 搜索
        rc = handleSearch(argc, argv);
    }
    else if (operation == 1) { // 鉴权
        rc = handleAuth(argc, argv);
    } 
    else if(operation == 2)  { //手动添加
        rc = handleManualAdd(argc, argv);
    }
    else if(operation == 3)  { //删除多条
        rc = handleDelSome(argc, argv);
    }
    else if(operation == 4)  { //删除全部
        rc = handleDelAll(argc, argv);
    }
    else if(operation == 5)  { //重新鉴权
        rc = handleReauth(argc, argv);
    }
    return rc;
}
