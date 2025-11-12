CONTIKI_PROJECT = data-compression
all: $(CONTIKI_PROJECT)

CONTIKI = ./..

PROJECT_SOURCEFILES += \
fire.c \
encoder.c \
decoder.c \
bitpack.c \
pipeline.c \
udp.c

CFLAGS += -DENERGEST_CONF_ON=1

include $(CONTIKI)/Makefile.include
