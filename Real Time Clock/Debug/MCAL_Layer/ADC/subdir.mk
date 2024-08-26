################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MCAL_Layer/ADC/ADC_prog.c 

OBJS += \
./MCAL_Layer/ADC/ADC_prog.o 

C_DEPS += \
./MCAL_Layer/ADC/ADC_prog.d 


# Each subdirectory must supply rules for building sources it contributes
MCAL_Layer/ADC/%.o: ../MCAL_Layer/ADC/%.c MCAL_Layer/ADC/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -O0 -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega32 -DF_CPU=8000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


