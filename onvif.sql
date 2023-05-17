create table db_version (
id INTEGER PRIMARY KEY,
version INTEGER,
remark TEXT
);

INSERT INTO DB_VERSION (VERSION, REMARK)
VALUES (1, "init");


create table ipcamera (
id INTEGER PRIMARY KEY,
username TEXT,
password TEXT,
uuid TEXT,
ip TEXT,
types TEXT,           -- 0 搜索 1 手动添加
webserver TEXT,
mediauri TEXT,
rtsp TEXT,
activeProfile INTEGER, -- 使用的profile 0 代表手动输入
hardware TEXT,
name TEXT,
authorization INTEGER, -- 0= 未鉴权, 1= 鉴权成功, 2=鉴权失败
show_on_dashboard INTEGER, -- 是否在dashboard上显示 0 不显示 1 显示
location TEXT,             -- 区域
visual_doorbell INTEGER,    -- 是否为可视化门铃 0 不是 1 是
support INTEGER,            -- 支持标记，0=不支持，1=支持, 2=未知待鉴权
width INTEGER,
height INTEGER,
record INTEGER,             -- 报警联动录像标记,1=录像，0=不录像
enable INTEGER,             --  0 = 未使能，实际监视时不显示此摄像头，1 = 使能，可以监视
flag INTEGER,               -- 选中显示标记,未使用，默认0
type INTEGER                -- 设备类型:0 = 支持onvif协议的ipc, 1 = 普通ipc, 2 = NVR设备
);

create table profile (
id INTEGER PRIMARY KEY,
d_id INTEGER,
name TEXT,
token TEXT,
videoEncoderConfigurationtoken TEXT,
encoding INTEGER, -- ns3__VideoEncoding__JPEG = 0, ns3__VideoEncoding__MPEG4 = 1, ns3__VideoEncoding__H264 = 2
h264Profile INTEGER, -- 0=Baseline
width INTEGER,
height INTEGER,
frameRateLimit INTEGER,
bitrateLimit INTEGER,
support INTEGER,
rtsp TEXT
);
