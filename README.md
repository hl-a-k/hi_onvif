# 概述
本工具针对IPC提供了以下功能：搜索、鉴权、手动添加、删除  
本工具的功能和原有的searchOnvif一样
但有以下几点改进：  

* 使用最新的gsoap, 解决某些ipc鉴权失败的问题
* 使用sqlite代替配置文件
* 更清晰的目录结构，方便后续维护
* 支持PTZ

# gsoap 使用

* 根据wsdl生成头文件  
`wsdl2h -o onvif.h -c -s -t ./typemap.dat http://www.onvif.org/ver10/device/wsdl/devicemgmt.wsdl http://www.onvif.org/ver10/network/wsdl/remotediscovery.wsdl http://www.onvif.org/ver10/media/wsdl/media.wsdl https://www.onvif.org/ver20/ptz/wsdl/ptz.wsdl
`

* 在onvif.h中，添加 #import "wsse.h" 

* 生成代码  
`soapcpp2 -c onvif.h -x -I../gsoap-2.8 -I../gsoap-2.8/gsoap -I../gsoap-2.8/gsoap/import/`

# 目录结构说明
<pre>
--build
--src
  |--app
     |--inc
     |--db.c
     |--main.c
  |--gsoap
--Makefile
--onvif.db3
--onvif.sql
</pre>
    build       编译产物
    src         源码
    db.c    数据库执行代码
    main.c      主要功能
    gsoap       gsoap生成的代码
    onvif.db3   空数据库
    onvif.sql   数据库创建脚本  

对比searchOnvif，代码的目录结构非常清晰，方便后续维护

# 使用说明

* search
`./hi_onvif 0 db_path`
* auth
`./hi_onvif 1 db_path id name username password`
* manualAdd
`./hi_onvif 2 db_path name username password rtsp`
* delSome
`./hi_onvif 3 db_path ids`
* delAll
`./hi_onvif 4 db_path`
* reauth
`./hi_onvif 5 db_path id`

# TODO

* 某些ipc,当设备的时间和panel不一致时。（设备ntp设置错误易导致该问题），鉴权失败。目前通过修改ipc时间解决该问题。 但对比onvif device manager, 该工具发送请求的时候，使用了设备的时间。因此，可以参考该做法，在鉴权前，获取设备时间（有相应的方法），提高该工具的适用性。