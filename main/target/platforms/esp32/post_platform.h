#pragma once

#if defined(HAVE_SDKCONFIG_H)
#include <sdkconfig.h>
#endif

#if !defined(TX_UNUSED_GPIO)
#define TX_UNUSED_GPIO 1
#endif
#if !defined(RX_UNUSED_GPIO)
#define RX_UNUSED_GPIO 3
#endif