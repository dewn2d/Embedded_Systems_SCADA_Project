################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../ece4220lab3.o 

C_SRCS += \
../Proj_SW.c 

OBJS += \
./Proj_SW.o 

C_DEPS += \
./Proj_SW.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gcc -I/home/students/dewn2d/nfsroot/usr/realtime/include -I/home/students/dewn2d/nfsroot/usr/src/linux24/include -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


