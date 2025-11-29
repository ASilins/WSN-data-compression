CONTIKI_PROJECT = data-compression
all: $(CONTIKI_PROJECT)

CONTIKI = ./..

PROJECTDIRS += network data compression
CFLAGS += -DENERGEST_CONF_ON=1

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

ifeq ($(ALGO),none)
CFLAGS += -DNONE=1
endif
# ===========================================
# ----------- Mote specific build files -----------
# ----- For root -----
ifeq ($(CLASS),root)
PROJECT_SOURCEFILES += \
	root_net.c \
	decoder.c
CFLAGS += -DROOT=1

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
	producer_net.c \
	pipeline.c \
	encoder.c
CFLAGS += -DPRODUCER=1

# Add algorithm dependend encoder
	ifeq ($(ALGO),sprintz)
	PROJECT_SOURCEFILES += sprintz_encoder.c
	endif
endif
# ====================
# ===========================================

include $(CONTIKI)/Makefile.include
