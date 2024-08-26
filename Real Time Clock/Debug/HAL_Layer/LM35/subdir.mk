################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HAL_Layer/LM35/LM35_prog.c 

OBJS += \
./HAL_Layer/LM35/LM35_prog.o 

C_DEPS += \
./HAL_Layer/LM35/LM35_prog.d 


# Each subdirectory must supply rules for building sources it contributes
HAL_Layer/LM35/%.o: ../HAL_Layer/LM35/%.c HAL_Layer/LM35/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -O0 -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega32 -DF_CPU=8000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


