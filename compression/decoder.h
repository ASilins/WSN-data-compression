#ifndef DECODER_H
#define DECODER_H

#include "project-conf.h"

#include "sys/log.h"

#if SPRINTZ
#include "fire.h"
#include "sprintz_decoder.h"

#endif /* SPRINTZ */

/* -------------------------------------------- */

void decode(const uint8_t *data, uint16_t datalen);

/* ============================================ */

#endif /* DECODER_H */