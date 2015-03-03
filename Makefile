#### Makefile for 3pi

PROJ = 3piLine

# MCU Settings
MCU = atmega328p 
LFUSE = F6
HFUSE = D9
EFUSE = 04

# Programmer
# Dragon
PROGRAMMER = dragon_isp
PORT = usb

SRCS = main.c
LIBDIR = -L/usr/local/pololu/lib
CFLAGS = -I/usr/local/pololu/include
CFLAGS += -fdata-sections -ffunction-sections
LIBS = -lpololu_$(MCU)
LDFLAGS += -Wl,-gc-sections

CC = avr-gcc
AVRDUDE = avrdude
RM := rm -rf

OBJS = $(patsubst %.c,%.o, $(SRCS))

# Dependencies

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
$(PROJ).lss \

MAP += \
$(PROJ).map

ELF += \
$(PROJ).elf

HEX += \
$(PROJ).hex \

EEP += \
$(PROJ).eep \

SIZEDUMMY += \
sizedummy \

# All Target
all: $(ELF) secondary-outputs 

# Tool invocations
$(ELF): $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	$(CC) -Wl,-Map,$(MAP) $(LDFLAGS) -mmcu=$(MCU) -o $(ELF) $(OBJS) $(USER_OBJS) $(LIBS) $(LIBDIR)
	@echo 'Finished building target: $@'
	@echo ' '

$(LSS): $(ELF)
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S $(ELF) >$(LSS)
	@echo 'Finished building: $@'
	@echo ' '

$(HEX): $(ELF)
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex $(ELF) $@
	@echo 'Finished building: $@'
	@echo ' '

$(EEP): $(ELF)
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex $(ELF) $(EEP)
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: $(ELF)
	@echo 'Invoking: Print Size'
	-avr-size --format=berkeley -t $(ELF)
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS) $(EEP) $(HEX) $(ELF) $(LSS) $(MAP) $(SIZEDUMMY)
	-@echo ' '

install: $(HEX)
	$(AVRDUDE) -B 1 -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -v -U flash:w:$(HEX)
	
fuses:
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -v -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

secondary-outputs: $(LSS) $(HEX) $(EEP) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

.c.o: 
	$(CC) -c $(CFLAGS) -mmcu=$(MCU) $< -o $@
	
