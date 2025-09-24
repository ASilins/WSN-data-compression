CONTIKI_PROJECT = data-compression
all: $(CONTIKI_PROJECT)

CONTIKI = ./..

CFLAGS += -DENERGEST_CONF_ON=1

include $(CONTIKI)/Makefile.include
