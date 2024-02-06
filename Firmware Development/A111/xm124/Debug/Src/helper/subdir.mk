################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/helper/acc_control_helper.c \
../Src/helper/acc_processing_helpers.c 

OBJS += \
./Src/helper/acc_control_helper.o \
./Src/helper/acc_processing_helpers.o 

C_DEPS += \
./Src/helper/acc_control_helper.d \
./Src/helper/acc_processing_helpers.d 


# Each subdirectory must supply rules for building sources it contributes
Src/helper/%.o Src/helper/%.su Src/helper/%.cyclo: ../Src/helper/%.c Src/helper/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L431xx -c -I"C:/Users/classic/Downloads/acconeer_xm124_a111-v2_15_2/xm124/Inc" -I../Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src-2f-helper

clean-Src-2f-helper:
	-$(RM) ./Src/helper/acc_control_helper.cyclo ./Src/helper/acc_control_helper.d ./Src/helper/acc_control_helper.o ./Src/helper/acc_control_helper.su ./Src/helper/acc_processing_helpers.cyclo ./Src/helper/acc_processing_helpers.d ./Src/helper/acc_processing_helpers.o ./Src/helper/acc_processing_helpers.su

.PHONY: clean-Src-2f-helper

