################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/input/input.c 

OBJS += \
./src/input/input.o 

C_DEPS += \
./src/input/input.d 


# Each subdirectory must supply rules for building sources it contributes
src/input/%.o: ../src/input/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -I"Z:\vm\test0423\openSSD_firmware\src" -I"Z:\vm\test0423\openSSD_firmware\src\cdma" -I"Z:\vm\test0423\openSSD_firmware\src\nvme" -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../openSSD_firmware_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


