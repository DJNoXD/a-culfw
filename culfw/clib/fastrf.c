#include <stdint.h>                     // for uint8_t

#include "board.h"                      // for HAS_FASTRF, TTY_BUFSIZE
#ifdef HAS_FASTRF
#include <string.h>                     // for strlen

#include "cc1100.h"                     // for cc1100_sendbyte, etc
#include "clock.h"                      // for ticks
#include "delay.h"                      // for my_delay_ms
#include "display.h"                    // for display_channel, DC, etc
#include "fastrf.h"
#include "fncollection.h"               // for EE_FASTRF_CFG

#ifdef HAS_MULTI_CC
#include "multi_CC.h"
#endif
uint8_t fastrf_on;

void
fastrf_func(char *in)
{
  uint8_t len = strlen(in);

  if(in[1] == 'r') {                // Init
#ifdef HAS_MULTI_CC
    set_RF_mode(RF_mode_fast);
#else
    ccInitChip(EE_FASTRF_CFG);
    ccRX();
    fastrf_on = 1;
#endif

  } else if(in[1] == 's') {         // Send

    CC1100_ASSERT;
    cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
    cc1100_sendbyte( len-2 );
    for(uint8_t i=2; i< len; i++)
      cc1100_sendbyte(in[i]);
    CC1100_DEASSERT;
    ccTX();
    while(cc1100_readReg(CC1100_TXBYTES) & 0x7f) // Wait for the data to be sent
      my_delay_ms(1);
    ccRX();                         // set reception again. MCSM1 does not work.

  } else {
#ifdef HAS_MULTI_CC
    set_RF_mode(RF_mode_off);
    fastrf_on = 0;
#else
    fastrf_on = 0;
#endif

  }
}

void
FastRF_Task(void)
{
#ifdef HAS_MULTI_CC
for(multiCC.instance = 0; multiCC.instance<HAS_MULTI_CC; multiCC.instance++) {
 if (is_RF_mode(RF_mode_fast)) {
#endif

  if(!fastrf_on)
#ifdef HAS_MULTI_CC
    break;
#else
    return;
#endif

  if(fastrf_on == 1) {

    static uint8_t lasttick;         // Querying all the time affects reception.
    if(lasttick != (uint8_t)ticks) {
      if(cc1100_readReg(CC1100_MARCSTATE) == MARCSTATE_RXFIFO_OVERFLOW) {
        ccStrobe(CC1100_SFRX);
        ccRX();
      }
      lasttick = (uint8_t)ticks;
    }
#ifdef HAS_MULTI_CC
    break;
#else
    return;
#endif
  }

  uint8_t len = cc1100_readReg(CC1100_RXFIFO);
  uint8_t buf[TTY_BUFSIZE];

  if(len < sizeof(buf)) {
    CC1100_ASSERT;
    cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    for(uint8_t i=0; i<len; i++)
      buf[i] = cc1100_sendbyte( 0 );
    CC1100_DEASSERT;

    display_channel=DISPLAY_USB;
    uint8_t i;
#ifdef HAS_MULTI_CC
    multiCC_prefix();
#endif
    for(i=0; i<len; i++)
      DC(buf[i]);
    DNL();
    display_channel=0xff;
  }
  fastrf_on = 1;
#ifdef HAS_MULTI_CC
 }
}
multiCC.instance = 0;
#endif
}

#endif
