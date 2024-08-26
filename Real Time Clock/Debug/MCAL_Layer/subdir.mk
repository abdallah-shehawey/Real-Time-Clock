################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MCAL_Layer/ADC_prog.c \
../MCAL_Layer/DIO_prog.c \
../MCAL_Layer/GIE_prog.c \
../MCAL_Layer/TIMER_program.c 

OBJS += \
./MCAL_Layer/ADC_prog.o \
./MCAL_Layer/DIO_prog.o \
./MCAL_Layer/GIE_prog.o \
./MCAL_Layer/TIMER_program.o 

C_DEPS += \
./MCAL_Layer/ADC_prog.d \
./MCAL_Layer/DIO_prog.d \
./MCAL_Layer/GIE_prog.d \
./MCAL_Layer/TIMER_program.d 


# Each subdirectory must supply rules for building sources it contributes
MCAL_Layer/%.o: ../MCAL_Layer/%.c MCAL_Layer/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -O0 -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega32 -DF_CPU=8000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


