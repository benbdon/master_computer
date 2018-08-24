#include "NU32.h"          // constants, funcs for startup and UART

void __ISR(_EXTERNAL_2_VECTOR, IPL2SOFT) Ext0ISR(void) { // step 1: the ISR
	while (!PORTDbits.RD7) {		// Pin D7 is the USER switch, low if pressed.
		;
	}
	NU32_LED1 = 0;                  // LED1 and LED2 on
	NU32_LED2 = 0;
	_CP0_SET_COUNT(0);
  
	while(_CP0_GET_COUNT() < 10000000) { ; } // delay for 10 M core ticks, 0.25 s
  
	NU32_LED1 = 1;                  // LED1 and LED2 off
	NU32_LED2 = 1;
	IFS0bits.INT2IF = 0;            // clear interrupt flag IFS0<3>
}

int main(void) {
  NU32_Startup(); // cache on, min flash wait, interrupts on, LED/button init, UART init
  __builtin_disable_interrupts(); // step 2: disable interrupts
  INTCONbits.INT2EP = 0;          // step 3: INT2 triggers on falling edge
  IPC2bits.INT2IP = 2;            // step 4: interrupt priority 2
  IPC2bits.INT2IS = 1;            // step 4: interrupt priority 1
  IFS0bits.INT2IF = 0;            // step 5: clear the int flag
  IEC0bits.INT2IE = 1;            // step 6: enable INT2 by setting IEC0<3>
  __builtin_enable_interrupts();  // step 7: enable interrupts
                                  // Connect RD7 (USER button) to INT0 (RD0)
  while(1) {
      ; // do nothing, loop forever
  }

  return 0;
}
