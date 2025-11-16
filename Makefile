CONTIKI_PROJECT = data-compression
all: $(CONTIKI_PROJECT)

CONTIKI = ./..

PROJECTDIRS += network data compression
CFLAGS += -DENERGEST_CONF_ON=0

# Default settings
CLASS ?= producer
ALGO ?= sprintz

# ---------- Algorithm build files ----------
ifeq ($(ALGO),sprintz)
PROJECTDIRS += sprintz
PROJECT_SOURCEFILES += \
	bitpack.c \
	fire.c 

CFLAGS += -DSPRINTZ=1
endif
# ===========================================
# ----------- Mote specific build files -----------
# ----- For sink -----
ifeq ($(CLASS),sink)
PROJECT_SOURCEFILES += \
	sink_net.c \
	decoder.c

# Add algorithm dependend decoder
	ifeq ($(ALGO),sprintz)
	PROJECT_SOURCEFILES += sprintz_decoder.c
	endif
endif
# ====================
# --- For producer ---
ifeq ($(CLASS),producer)
PROJECTDIRS += pipeline
PROJECT_SOURCEFILES += \
	udp.c \
	pipeline.c \
	encoder.c

# Add algorithm dependend encoder
	ifeq ($(ALGO),sprintz)
	PROJECT_SOURCEFILES += sprintz_encoder.c
	endif
endif
# ====================
# ===========================================

include $(CONTIKI)/Makefile.include
