#ifndef PIPELINE_H
#define PIPELINE_H

#include "contiki.h"
#include "project-conf.h"
#include "timeseries_data.h"

#include "sys/log.h"
#include "dev/button-sensor.h"

#include "encoder.h"
#include "udp.h"

extern struct process main_pipeline_process;
extern struct process pipeline_process;

#endif /* PIPELINE_H */