#pragma once
#include "api/common.h"

void api_interrupt_init(void);
int api_interrupt_trigger(api_int_id_t id);
