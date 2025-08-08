SDK_DEMO_PATH ?= ./src
BL_SDK_BASE ?= ./SDK

export BL_SDK_BASE

CHIP ?= bl702
BOARD ?= bl702rd
CROSS_COMPILE ?= riscv64-unknown-elf-
CONFIG_USB_HS ?=n
COMPORT ?= COM15

# add custom cmake definition
#cmake_definition+=-Dxxx=sss

include $(BL_SDK_BASE)/project.build

all:build

flash:
	E:\Bouffalo\bflb_flash_tool\bflb_mcu_tool.exe --chipname=$(CHIP) --port=$(COMPORT) --xtal=32M --firmware=.\build\build_out\project_$(CHIP).bin
