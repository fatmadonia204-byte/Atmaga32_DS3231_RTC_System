################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include mcal/twi/subdir.mk
-include mcal/timer0/subdir.mk
-include mcal/dio/subdir.mk
-include hal/rtc/subdir.mk
-include hal/lcd/subdir.mk
-include hal/keypad/subdir.mk
-include hal/buzzer/subdir.mk
-include app/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
RRTTCC.lss \

FLASH_IMAGE += \
RRTTCC.hex \

SIZEDUMMY += \
sizedummy \


# All Target
all: RRTTCC.elf secondary-outputs

# Tool invocations
RRTTCC.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,RRTTCC.map -mmcu=atmega32 -o "RRTTCC.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

RRTTCC.lss: RRTTCC.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S RRTTCC.elf  >"RRTTCC.lss"
	@echo 'Finished building: $@'
	@echo ' '

RRTTCC.hex: RRTTCC.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex RRTTCC.elf  "RRTTCC.hex"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: RRTTCC.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega32 RRTTCC.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(FLASH_IMAGE)$(ELFS)$(OBJS)$(ASM_DEPS)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(LSS)$(C_DEPS) RRTTCC.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
