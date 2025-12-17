#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "sys/log.h"
#include "project-conf.h"
#include "sys/energest.h"

/* Algorithm-specific includes */
#if SPRINTZ
#include "sprintz_decoder.h"
#include "sprintz/fire.h"
#include "sprintz/sprintz_decoder.h"
#endif

#if PLA
#include "pla_decoder.h"
#endif

/**
 * @brief Main decode function - selects algorithm based on compile config
 */
void decode(const uint8_t *data, uint16_t datalen);

#endif /* DECODER_H */
