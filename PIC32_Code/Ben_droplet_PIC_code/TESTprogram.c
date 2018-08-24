//#define NU32_STANDALONE		// uncomment if program is standalone, not bootloaded
#include "NU32.h"						// config bits, constants, funcs for startup and UART
#include "LCD.h"


#define MAX_MESSAGE_LENGTH 200
#define PULSETIME 900

// Count the number of drops created on LCD
volatile int drop_counter = 0;

// ***********************************************************************************************
// If USER button has been pressed, create a droplet (not for video recording)
// ***********************************************************************************************
void __ISR(_EXTERNAL_2_VECTOR, IPL2SOFT) Ext2ISR(void) { // step 1: the ISR

	// Wait until USER button is released, then wait 5M core ticks before clearing interrupt flag
	while(!PORTDbits.RD7) {		// Pin D7 is the USER switch, low if pressed.
		;
	}

	int counter2=0;
	while(counter2<5000000) { // debounce
		counter2++;
	}

	//+Vcc: set L1 to LOW, L2 to HIGH, PWMA to >0
	OC1RS = 4000;
	LATDbits.LATD2 = 0; //
	LATDbits.LATD1 = 1;

	//Remain +Vcc for PULSETIME microseconds
	int counter3=0;
	while(counter3<(PULSETIME*40)) {
		counter3++;
	}

	//-Vcc: set L1 to HIGH, L2 to LOW, PWMA to >0
	OC1RS = 4000;
	LATDbits.LATD2 = 1;
	LATDbits.LATD1 = 0;


	// Increment drop counter
	drop_counter++;


	// Create message for LCD
	char message[50];

	// Display properties on LCD
	LCD_Clear();
	LCD_Move(0,0);
	sprintf(message, "Drops: %d", drop_counter);
	NU32_WriteUART3(message);



	// Clear interrupt flag
	IFS0CLR = 1 << 11;
}

int main(void) {

	// Cache on, min flash wait, interrupts on, LED/button init, UART init
	NU32_Startup();

	// Setup LCD and create message to be sent
	LCD_Setup();
	char message[MAX_MESSAGE_LENGTH];

	// =============================================================================================
	// USER Button Interrupt
	// =============================================================================================
	__builtin_disable_interrupts(); 	// step 2: disable interrupts
	INTCONCLR = 0x1;               	 	// step 3: INT0 triggers on falling edge
	IPC2CLR = 0x1F << 24;           	// step 4: clear the 5 pri and subpri bits
	IPC2 |= 9 << 24;               	 	// step 4: set priority to 2, subpriority to 1
	IFS0bits.INT2IF = 0;           	 	// step 5: clear the int flag, or IFS0CLR=1<<3
	IEC0SET = 1 << 11;              	// step 6: enable INT0 by setting IEC0<3>
  __builtin_enable_interrupts();  	// step 7: enable interrupts

	// =============================================================================================
	// PWM and digital output for piezoelectric droplet generator
	// =============================================================================================
	OC1CONbits.OCTSEL = 1;  			 			// Select Timer3 for comparison

	T3CONbits.TCKPS = 0;     						// Timer3 prescaler N=1 (1:1)
	PR3 = 3999;              						// period = (PR3+1) * N * 12.5 ns = 20 kHz
	TMR3 = 0;                						// initial TMR3 count is 0

	OC1CONbits.OCM = 0b110; 						// PWM mode with no fault pin; other OC1CON bits are defaults
	OC1RS = 0;           								// duty cycle = OC1RS/(PR3+1) = 75%
	OC1R = 0;             							// initialize before turning OC1 on; afterward it is read-only

	T3CONbits.ON = 1;        						// turn on Timer3
	OC1CONbits.ON = 1;       						// turn on OC1

	// Set A10/A2 to digital output pins
	TRISDbits.TRISD2 = 0;							// RA10 is an output pin
	TRISDbits.TRISD1 = 0;								// RA2 is an output pin

	//-Vcc: set L1 to HIGH, L2 to LOW, PWMA to >0
	OC1RS = 4000;
	LATDbits.LATD2 = 1;
	LATDbits.LATD1 = 0;

	// =============================================================================================
	// Keep program running to look for command from master to start record procedure
	// =============================================================================================
	while(1) {
		TRISF = 0xFFFC;        // Pins 0 and 1 of Port F are LED1 and LED2.  Clear
							   // bits 0 and 1 to zero, for output.  Others are inputs.
		LATFbits.LATF0 = 0;    // Turn LED1 on and LED2 off.  These pins sink current
		LATFbits.LATF1 = 1;    // on the NU32, so "high" (1) = "off" and "low" (0) = "on"

		while (1) {
			LATFINV = 0x0003;    // toggle LED1 and LED2; same as LATFINV = 0x3;
			int counter1=0;
			while(counter1<20000000) { // debounce
				counter1++;
			}
		}
	}

	return 0;
}
