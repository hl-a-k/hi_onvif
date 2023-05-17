### project related, you may need change this part.
TOOL_PREFIX=/opt/fsl-imx-fb/4.14-sumo/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-
target_sdk_sysroot = /opt/fsl-imx-fb/4.14-sumo/sysroots/cortexa9hf-neon-poky-linux-gnueabi
CC=$(TOOL_PREFIX)gcc -march=armv7-a -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 --sysroot=$(target_sdk_sysroot)
LD=$(TOOL_PREFIX)gcc -march=armv7-a -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 --sysroot=$(target_sdk_sysroot)
SSL_HEAD := -I ../../third_party/src/library/openssl/openssl-1.1.1j/include
SSL_LIB := -L../../third_party/src/library/openssl/openssl-1.1.1j
###

TARGET_EXEC := hi_onvif

BUILD_DIR := ./build
SRC_DIRS := ./src



# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. Make will incorrectly expand these otherwise.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(SSL_HEAD)

LDFLAGS := $(SSL_LIB)  -lcrypto -lssl -lsqlite3 
CFLAGS := -g -DWITH_DOM -DWITH_OPENSSL

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(INC_FLAGS) $(CFLAGS) -c $< -o $@


interface:
	@echo no interface needed for $(TARGET_EXEC)

interface-clean:
	@echo no interface-clean needed for $(TARGET_EXEC)

romfs: $(BUILD_DIR)/$(TARGET_EXEC)
	@echo install $(TARGET_EXEC) to $(ROOTFS_DIR)/usr/app/
	$(MKDIR) $(ROOTFS_DIR)/usr/app/
	$(CP) $(BUILD_DIR)/$(TARGET_EXEC) $(ROOTFS_DIR)/usr/app/

.PHONY: clean interface interface-clean romfs
clean:
	rm -r $(BUILD_DIR)

