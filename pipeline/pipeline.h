#ifndef PIPELINE_H
#define PIPELINE_H

#include "contiki.h"
#include "project-conf.h"
#include "timeseries_data.h"

#include "sys/log.h"

#include "sys/energest.h"

#include "encoder.h"
#include "producer_net.h"

extern struct process main_pipeline_process;
extern process_event_t START_PIPELINE_EVENT;

#endif /* PIPELINE_H */