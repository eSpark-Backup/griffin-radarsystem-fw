################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/use_cases/example_apps/example_surface_velocity.c 

OBJS += \
./Src/use_cases/example_apps/example_surface_velocity.o 

C_DEPS += \
./Src/use_cases/example_apps/example_surface_velocity.d 


# Each subdirectory must supply rules for building sources it contributes
Src/use_cases/example_apps/%.o Src/use_cases/example_apps/%.su Src/use_cases/example_apps/%.cyclo: ../Src/use_cases/example_apps/%.c Src/use_cases/example_apps/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu99 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L431xx -c -I"C:/Users/Windows/Documents/Github/griffin-radarsystem-fw/Firmware Development/A121/Inc" -I../Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src-2f-use_cases-2f-example_apps

clean-Src-2f-use_cases-2f-example_apps:
	-$(RM) ./Src/use_cases/example_apps/example_surface_velocity.cyclo ./Src/use_cases/example_apps/example_surface_velocity.d ./Src/use_cases/example_apps/example_surface_velocity.o ./Src/use_cases/example_apps/example_surface_velocity.su

.PHONY: clean-Src-2f-use_cases-2f-example_apps

