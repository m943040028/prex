#
# Automatically generated file. Don't edit
#
_CONFIG_MK_=1
ARCH=ppc
PLATFORM=taihu
PROFILE=taihu
_GNUC_=1
CC=powerpc-amcc-elf-gcc
CPP=powerpc-amcc-elf-cpp
AS=powerpc-amcc-elf-as
LD=powerpc-amcc-elf-ld
AR=powerpc-amcc-elf-ar
OBJCOPY=powerpc-amcc-elf-objcopy
OBJDUMP=powerpc-amcc-elf-objdump
STRIP=powerpc-amcc-elf-strip
GCCFLAGS+= -fno-stack-protector
GCCFLAGS+= -mcpu=powerpc -m32 -mno-eabi -msoft-float
ASFLAGS+= -mregnames
CONFIG_LOADER_TEXT=0xfff80000
CONFIG_KERNEL_TEXT=0x00080000
CONFIG_BOOTIMG_BASE=0x00102000
CONFIG_SYSPAGE_BASE=0x00000000
CONFIG_HZ=1000
CONFIG_TIME_SLICE=50
CONFIG_OPEN_MAX=16
CONFIG_BUF_CACHE=32
CONFIG_FS_THREADS=4
CONFIG_MPC750=y
CONFIG_CACHE=y
CONFIG_BOOTDISK=y
CONFIG_PPC_OEA=y
CONFIG_POSIX=y
CONFIG_CMDBOX=y
CONFIG_KD=y
CONFIG_DIAG_SERIAL=y
CONFIG_FIFOFS=y
CONFIG_DEVFS=y
CONFIG_RAMFS=y
CONFIG_ARFS=y
CONFIG_PM=y
CONFIG_PM_PERFORMANCE=y
CONFIG_PM=y
CONFIG_CONS=y
CONFIG_SERIAL=y
CONFIG_NS16550=y
CONFIG_NULL=y
CONFIG_ZERO=y
CONFIG_RAMDISK=y
CONFIG_NS16550_BASE=0xef600300
CONFIG_NS16550_IRQ=4
CONFIG_MC146818_BASE=0x70
CONFIG_CMD_CAT=y
CONFIG_CMD_CLEAR=y
CONFIG_CMD_CP=y
CONFIG_CMD_DATE=y
CONFIG_CMD_DMESG=y
CONFIG_CMD_ECHO=y
CONFIG_CMD_FREE=y
CONFIG_CMD_HEAD=y
CONFIG_CMD_HOSTNAME=y
CONFIG_CMD_KILL=y
CONFIG_CMD_LS=y
CONFIG_CMD_MKDIR=y
CONFIG_CMD_MORE=y
CONFIG_CMD_MV=y
CONFIG_CMD_NICE=y
CONFIG_CMD_PRINTENV=y
CONFIG_CMD_PS=y
CONFIG_CMD_PWD=y
CONFIG_CMD_RM=y
CONFIG_CMD_RMDIR=y
CONFIG_CMD_SH=y
CONFIG_CMD_SLEEP=y
CONFIG_CMD_SYNC=y
CONFIG_CMD_TOUCH=y
CONFIG_CMD_UNAME=y
CONFIG_CMD_DISKUTIL=y
CONFIG_CMD_INSTALL=y
CONFIG_CMD_PMCTRL=y
CONFIG_CMD_KTRACE=y
CONFIG_CMD_LOCK=y
CONFIG_CMD_DEBUG=y
