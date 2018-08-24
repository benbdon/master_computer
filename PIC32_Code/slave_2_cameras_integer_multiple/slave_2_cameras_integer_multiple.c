//#define NU32_STANDALONE		// uncomment if program is standalone, not bootloaded
#include "NU32.h"						// config bits, constants, funcs for startup and UART
#include "LCD.h"


#define MAX_MESSAGE_LENGTH 20


// Master (c++ code) will be in charge of this program
// 	- Number of images to grab and fps
//	- Pulsetime for droplet generation
//	- Delay time before create droplet
//
// Will receive message from master by serial communication in one string
// Upon receival, will set properties and immediately start process
// Once process has finished, will wait for another READUART command from master
// - Serial message: [NUMIMAGES_Side, FPS_Side, NUMIMAGES_Top, FPS_Top, PULSETIME, DELAYTIME]


// Synchronization with PPOD controller
//
// CN17 - RF4 - 5V Tolerant


// Exsync Trigger: LOW trigger tells camera to record one frame 
//
// CPU running at 40 MHz
// Trigger LOW for 100 clock cycles
// 
// LOW:  -0.3 V TO 0.8 V {0.0 V}
// HIGH:  2.0 V to 5.5 V {3.3 V}


// Piezoelectric waveform: -Vcc changes to +Vcc for T microseconds, then back to -Vcc
//
// PWMA = 20 kHz PWM - Timer3 - OC1 (D0)
// L1 = Digital Output RD2
// L2 = Digital Output RD1
// VM = 24V / 600mA
//
// CPU running at 40 MHz
//
// FWD-SPD: L1 LOW, L2 HIGH
// BCK-SPD: L1 HIGH, L2 LOW
// 
// LOW: -0.3 V TO 1.5 V {0.0 V}
// HIGH: 2.3 V TO 7.0 V {3.3 V}


// ***********************************************************************************************
// Variables to be accessed from multiple functions
// ***********************************************************************************************

// Hold the port F read
unsigned int oldF = 0, newF = 0;

// Create Record flag and turn OFF
volatile unsigned char RecordFlag = 0;

// Create Record flag and turn OFF
volatile unsigned char TriggerFlag_Side = 0;

// Properties set by master
int NUMIMAGES_Side, FPS_Side, INTEGER_MULTIPLE, PULSETIME = 900;
volatile float DELAYTIME;

// Keep track of number of ExSync signals sent to camera
volatile int CurrentExSync_Side = 0;

// Count the number of drops created on LCD
volatile int drop_counter = 0;


// ***********************************************************************************************
// ISR using Change Notification for synchronization with PPOD controller
// ***********************************************************************************************
void __ISR(_CHANGE_NOTICE_VECTOR, IPL3SOFT) CNISR(void) { // INT step 1
  
	// Since pins on port F are being monitored by CN, must read it to allow continued functioning
  newF = PORTF;

	// Save the current values for future use
  oldF = newF;


	// =============================================================================================
	// If RecordFlag is on, start the procedure of recording droplet bouncing
	// =============================================================================================
	if (RecordFlag == 1) {

  	// Turn TriggerFlag ON to let ISRs know to create ExSync signals
	  TriggerFlag_Side = 1;


		// -------------------------------------------------------------------------------------------
		// Delay DELAYTIME seconds for phase relationship of droplet contacting bath
		// -------------------------------------------------------------------------------------------
		int counter1=0;
		int delay = DELAYTIME*40000000;
		while( counter1 < delay ) {
			counter1++;
		}


		// -------------------------------------------------------------------------------------------
		// Create droplet
		// -------------------------------------------------------------------------------------------

		//+Vcc: set L1 to LOW, L2 to HIGH, PWMA to >0
		OC1RS = 4000;
		LATDbits.LATD2 = 0;
		LATDbits.LATD1 = 1;

		//Remain +Vcc for PULSETIME microseconds
		int counter2=0;
		while( counter2 < (PULSETIME*40) ) {
			counter2++;
		}

		//-Vcc: set L1 to HIGH, L2 to LOW, PWMA to >0
		OC1RS = 4000;
		LATDbits.LATD2 = 1;
		LATDbits.LATD1 = 0;


		// -------------------------------------------------------------------------------------------
		// Wait until both ExSync signals have been sent
		// -------------------------------------------------------------------------------------------
		while( CurrentExSync_Side < (NUMIMAGES_Side-1) ) {

			// Turn TriggerFlag_Side OFF when CurrentExSync_Side is the same as NUMIMAGES_Side
			if( CurrentExSync_Side == (NUMIMAGES_Side-1) ) {
				TriggerFlag_Side = 0;
			}

		}



		// Increment drop counter
		drop_counter++;


		// Create message for LCD
		char message[50];

		// Display properties on LCD
		LCD_Clear();
		LCD_Move(0,0);
		sprintf(message, "Drops: %d", drop_counter);
		LCD_WriteString(message);


		
		// -------------------------------------------------------------------------------------------
		// Turn TriggerFlag and RecordFlag OFF, as well as resetting current ExSync signals
		// -------------------------------------------------------------------------------------------
		TriggerFlag_Side = 0;			// needs to be done again or after the first avi, trackcam only records 1 frame

		RecordFlag = 0;

		CurrentExSync_Side = 0;



		// Sleep for 2 seconds - helps solves double drop production problem
		int counter3=0;
		while(counter3<(2*40000000)) {
			counter3++;
		}



	}


	// Clear the interrupt flag
	IFS1bits.CNIF = 0;
}




// ***********************************************************************************************
// ISR using Timer2 to generate ExSync signal for Mikrotron and TrackCam cameras
// ***********************************************************************************************
void __ISR(_TIMER_2_VECTOR, IPL5SOFT) ExSyncSide(void) {

	// =============================================================================================
	// If TriggerFlag_Side is on, generate ExSync signals
	// =============================================================================================
	if( TriggerFlag_Side == 1 ) {

		// Send ExSync signals to both cameras for the first frame
		if( CurrentExSync_Side == 0 ) {
			// Set A3/A4 to LOW
			LATBbits.LATB3 = 0;
			LATBbits.LATB4 = 0;

			// Remain LOW for 100 clock cycles
			int counter1 = 0;
			while( counter1 < 100 ) {
				counter1++;
			}

			// Set A3/A4 to HIGH
			LATBbits.LATB3 = 1;
			LATBbits.LATB4 = 1;
		}
		// If integer multiple of side-view camera, send ExSync signals to both camera
		else if( (CurrentExSync_Side % INTEGER_MULTIPLE) == 0 ) {
			// Set A3/A4 to LOW
			LATBbits.LATB3 = 0;
			LATBbits.LATB4 = 0;

			// Remain LOW for 100 clock cycles
			int counter1 = 0;
			while( counter1 < 100 ) {
				counter1++;
			}

			// Set A3/A4 to HIGH
			LATBbits.LATB3 = 1;
			LATBbits.LATB4 = 1;
		}
		// If not integer multiple of side-view camera, send ExSync signals to side-view camera only
		else {
			// Set A3 to LOW
			LATBbits.LATB3 = 0;

			// Remain LOW for 100 clock cycles
			int counter1 = 0;
			while( counter1 < 100 ) {
				counter1++;
			}

			// Set A3 to HIGH
			LATBbits.LATB3 = 1;
		}


		// Increment current ExSync signal
		CurrentExSync_Side++;
	}

	// Clear interrupt flag
	IFS0bits.T2IF = 0;
}




// ***********************************************************************************************
// If USER button has been pressed, create a droplet (not for video recording)
// ***********************************************************************************************
void __ISR(_EXTERNAL_2_VECTOR, IPL2SOFT) Ext2ISR(void) { // step 1: the ISR

	// Wait until USER button is released, then wait 5M core ticks before clearing interrupt flag
	while(!PORTDbits.RD7) {		// Pin D7 is the USER switch, low if pressed.
		;
	}

	int counter=0;
	while(counter<5000000) {
		counter++;
	}
	
	//+Vcc: set L1 to LOW, L2 to HIGH, PWMA to >0
	OC1RS = 4000;
	LATDbits.LATD2 = 0;
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
	char message[MAX_MESSAGE_LENGTH];

	// Display properties on LCD
	LCD_Clear();
	LCD_Move(0,0);
	sprintf(message, "Drops: %d", drop_counter);
	LCD_WriteString(message);  
	
	counter = 0;
	while (counter<5000000) {
		counter++;
	}

	// Clear interrupt flag
	IFS0bits.INT2IF = 0;
}







int main(void) {

	// Cache on, min flash wait, interrupts on, LED/button init, UART init
	NU32_Startup();

	// Setup LCD and create message to be sent
	LCD_Setup();
	char message[MAX_MESSAGE_LENGTH];

	// Local variables for determining and setting Timer interrupts based on FPS_Side
	//int Time1_Side = 0, N_Side = 0, PR_Side = 0, TCKPS_Side = 0;


	// =============================================================================================
	// Change Notification Digital Input Interrupt from PPOD
	// =============================================================================================
	//CNPUEbits.CNPUE17 = 0;  						// CN17/RF4 input has no internal pull-up

	//oldF = PORTF;           						// all pins of port F are inputs, by default

	//__builtin_disable_interrupts(); 		// step 1: disable interrupts
	//CNCONbits.ON = 1;               		// step 2: configure peripheral: turn on CN
	//CNENbits.CNEN17 = 1; 								//         listen to CN17/RF4
	//IPC6bits.CNIP = 3;              		// step 3: set interrupt priority
	//IPC6bits.CNIS = 2;              		// step 4: set interrupt subpriority
	//IFS1bits.CNIF = 0;              		// step 5: clear the interrupt flag
	//IEC1bits.CNIE = 1;              		// step 6: enable the _CN interrupt
	//__builtin_enable_interrupts();  		// step 7: CPU enabled for mvec interrupts


	// =============================================================================================
	// USER Button Interrupt
	// =============================================================================================
	__builtin_disable_interrupts(); 	// step 2: disable interrupts
	INTCONbits.INT2EP = 0;              // step 3: INT2 triggers on falling edge
	IPC2bits.INT2IP = 2;           		// step 4: interrupt priority to 2
	IPC2bits.INT2IS = 1;               	// step 4: interrupt subpriority to 1
	IFS0bits.INT2IF = 0;           	 	// step 5: clear the int flag
	IEC0bits.INT2IE = 1;              	// step 6: enable INT2 by setting IEC0<11>
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

	// Set D2/A2 to digital output pins
	TRISDbits.TRISD2 = 0;							// RA10 is an output pin
	TRISDbits.TRISD1 = 0;								// RA2 is an output pin
													
	//-Vcc: set L1 to HIGH, L2 to LOW, PWMA to >0
	OC1RS = 4000;
	LATDbits.LATD2 = 1;
	LATDbits.LATD1 = 0;


	// =============================================================================================
	// TrackCam ExSync Trigger - Side (B3), Top (B4)
	// =============================================================================================

	// Digital output pin
	//TRISBbits.TRISB3 = 0;
	//TRISBbits.TRISB4 = 0;

	// Set to HIGH
	//LATBbits.LATB3 = 1;
	//LATBbits.LATB4 = 1;



	// =============================================================================================
	// Keep program running to look for command from master to start record procedure
	// =============================================================================================
	while(1) {
		NU32_LED2 = !NU32_LED2; // toggle LED1
		int j;
		for (j = 0; j < 1000000; j++) { // number is 1 million
			while (!PORTDbits.RD7) {
				;   // Pin D7 is the USER switch, low (FALSE) if pressed.
			}
		}
		/*
		// Get message from computer
		NU32_ReadUART3(message, MAX_MESSAGE_LENGTH);

		Serial message: [NUMIMAGES_Side, FPS_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME]
		sscanf(message, "%d%*c %d%*c %d%*c %d%*c %f", &NUMIMAGES_Side, &FPS_Side, &INTEGER_MULTIPLE, &PULSETIME, &DELAYTIME);		//%*c reads in comma and ignores it


		// -------------------------------------------------------------------------------------------
		// Side Camera Interrupt -  NUMIMAGES_Side Hz - Timer2
		// -------------------------------------------------------------------------------------------

		// Calculate pre-scaler based on FPS_Side
		Time1_Side = (((1.0/FPS_Side*1.0)*1e9)/12.5);
		N_Side = ceil( (Time1_Side*1.0/65535.0) );

		// Select best pre-scaler of 1, 2, 4, 8, 16, 32, 64, or 256 
		if( N_Side == 1 ) {
			N_Side = 1;
			TCKPS_Side = 0;
		} else if( N_Side <= 2 ) {
			N_Side = 2;
			TCKPS_Side = 1;
		} else if( N_Side <= 4 ) {
			N_Side = 4;
			TCKPS_Side = 2;
		} else if( N_Side <= 8 ) {
			N_Side = 8;
			TCKPS_Side = 3;
		} else if( N_Side <= 16 ) {
			N_Side = 16;
			TCKPS_Side = 4;
		} else if( N_Side <= 32 ) {
			N_Side = 32;
		TCKPS_Side = 5;
		} else if( N_Side <= 64 ) {
		N_Side = 64;
			TCKPS_Side = 6;
		} else {
			N_Side = 256;
			TCKPS_Side = 7;
		}

		// Calculate period register based on selected prescaler
		PR_Side = (Time1_Side / N_Side) - 1;



		__builtin_disable_interrupts(); 	// INT step 2: disable interrupts at CPU

											// INT step 3: 	setup TMR2 to call ISR at frequency of 5 kHz
		PR2 = PR_Side;						// set period register to 16,000
		TMR2 = 0;							// initialize count to 0
		T2CONbits.TCKPS = TCKPS_Side;		// 	set prescaler to 1:1
		T2CONbits.ON = 1; 					// turn on Timer2
		IPC2bits.T2IP = 5; 					// INT step 4: 	priority 5
		IPC2bits.T2IS = 2; 					// subpriority 2
		IFS0bits.T2IF = 0; 					// INT step 5: 	clear interrupt flag
		IEC0bits.T2IE = 1; 					// INT step 6: 	enable interrupt

		__builtin_enable_interrupts(); 		// INT step 7: 	enable interrupts at CPU



		

		// Turn RecordFlag ON since we have received command from master
		RecordFlag = 1;

		// Display properties on LCD
		LCD_Clear();
		LCD_Move(0,0);
		sprintf(message, "%d, %d, %d", FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE);
		LCD_WriteString(message);                     	

		LCD_Move(1,0);
		sprintf(message, "%d, %f", PULSETIME, DELAYTIME);
		LCD_WriteString(message);
		*/
	}

	return 0;
}

