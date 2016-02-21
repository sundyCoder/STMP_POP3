################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../md5.c \
../pop3_main.c \
../pop3_ser.c \
../tcp.c 

OBJS += \
./md5.o \
./pop3_main.o \
./pop3_ser.o \
./tcp.o 

C_DEPS += \
./md5.d \
./pop3_main.d \
./pop3_ser.d \
./tcp.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


