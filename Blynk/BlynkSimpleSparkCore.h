/**
 * @file       BlynkSimpleSparkCore.h
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Mar 2015
 * @brief
 *
 */

#ifndef BlynkSimpleSparkCore_h
#define BlynkSimpleSparkCore_h

#warning "You should use BlynkSimpleParticle.h now. BlynkSimpleSparkCore.h will be removed"

#include "BlynkParticle.h"

static BlynkTransportParticle _blynkTransport;
BlynkParticle Blynk(_blynkTransport);

#include "BlynkWidgets.h"

#endif
