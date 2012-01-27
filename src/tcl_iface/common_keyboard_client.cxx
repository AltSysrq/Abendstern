/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/tcl_iface/common_keyboard_client.hxx
 */

/*
 * common_keyboard_client.cxx
 *
 *  Created on: 25.02.2011
 *      Author: jason
 */

class Ship;

#include "common_keyboard_client.hxx"
#include "src/control/human_controller.hxx"
#include "src/control/hc_conf.hxx"

#define FUN(name) \
static void action_##name(Ship*, ActionDatum& dat) { \
  ((CommonKeyboardClient*)dat.ptr)->name(); \
}
FUN(exit)
FUN(frameXframe)
FUN(slow)
FUN(fast)
FUN(halt)
FUN(statsOn)
FUN(statsOff)

CommonKeyboardClient::CommonKeyboardClient() {
}

void CommonKeyboardClient::hc_conf_bind() {
  ActionDatum ad;
  ad.ptr = this;
  DigitalAction da_exit = { ad, false, false, action_exit, NULL },
                da_fxf  = { ad, false, false, action_frameXframe, NULL },
                da_slow = { ad, false, false, action_slow, NULL },
                da_fast = { ad, false, false, action_fast, NULL },
                da_halt = { ad, false, false, action_halt, NULL },
                da_stat = { ad, false, false, action_statsOn, action_statsOff };
  hc_conf::bind(da_exit, "__ exit");
  hc_conf::bind(da_fxf,  "__ frameXframe");
  hc_conf::bind(da_slow, "__ slow");
  hc_conf::bind(da_fast, "__ fast");
  hc_conf::bind(da_halt, "__ halt");
  hc_conf::bind(da_stat, "stats");
}
