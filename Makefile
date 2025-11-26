CONTIKI_PROJECT = data-compression
all: $(CONTIKI_PROJECT)

CONTIKI = ./..

PROJECTDIRS += network data compression
CFLAGS += -DENERGEST_CONF_ON=0

# Default settings
CLASS ?= producer
ALGO ?= sprintz

# ============================================
# ---------- Algorithm build files ----------
# ============================================

# ----- Sprintz (lossless) -----
ifeq ($(ALGO),sprintz)
PROJECTDIRS += sprintz
PROJECT_SOURCEFILES += \
	bitpack.c \
	fire.c 

CFLAGS += -DSPRINTZ=1
endif

# ----- PLA (lossy, with error threshold) -----
ifeq ($(ALGO),pla)
PROJECTDIRS += pla
CFLAGS += -DPLA=1
endif

# ============================================
# ----------- Mote specific build files -----
# ============================================

# ----- For sink -----
ifeq ($(CLASS),sink)
PROJECT_SOURCEFILES += \
	sink_net.c \
	decoder.c

# Add algorithm dependent decoder
ifeq ($(ALGO),sprintz)
PROJECT_SOURCEFILES += sprintz_decoder.c
endif

ifeq ($(ALGO),pla)
PROJECT_SOURCEFILES += pla_decoder.c
endif
endif

# ----- For producer -----
ifeq ($(CLASS),producer)
PROJECTDIRS += pipeline
PROJECT_SOURCEFILES += \
	udp.c \
	pipeline.c \
	encoder.c

# Add algorithm dependent encoder
ifeq ($(ALGO),sprintz)
PROJECT_SOURCEFILES += sprintz_encoder.c
endif

ifeq ($(ALGO),pla)
PROJECT_SOURCEFILES += pla_encoder.c
endif
endif

# ============================================
include $(CONTIKI)/Makefile.include
