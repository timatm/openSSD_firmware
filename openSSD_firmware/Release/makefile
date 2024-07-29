################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/nvme/subdir.mk
-include src/nmc/subdir.mk
-include src/monitor/subdir.mk
-include src/input/subdir.mk
-include src/cdma/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
ELFSIZE += \
openSSD_firmware.elf.size \


# All Target
all: pre-build main-build

# Main-build Target
main-build: openSSD_firmware.elf secondary-outputs

# Tool invocations
openSSD_firmware.elf: $(OBJS) ../src/lscript.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM v7 gcc linker'
	arm-none-eabi-gcc -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -Wl,-build-id=none -specs=Xilinx.spec -Wl,-T -Wl,../src/lscript.ld -L../../openSSD_firmware_bsp/ps7_cortexa9_0/lib -o "openSSD_firmware.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

openSSD_firmware.elf.size: openSSD_firmware.elf
	@echo 'Invoking: ARM v7 Print Size'
	arm-none-eabi-size openSSD_firmware.elf  |tee "openSSD_firmware.elf.size"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(S_UPPER_DEPS)$(C_DEPS)$(ELFSIZE) openSSD_firmware.elf
	-@echo ' '

pre-build:
	-a9-linaro-pre-build-step
	-@echo ' '

secondary-outputs: $(ELFSIZE)

.PHONY: all clean dependents
.SECONDARY: main-build pre-build

-include ../makefile.targets
