#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "inc/tm4c123gh6pm.h"
#include "Lab10.h"

//*****************************************************************************
//
//! In this project we use ADC0, SS3 to measure the data from Pin PE3 
//! of the microcontroller. The ADC sampling is triggered by software at a 
//! frequency of 1000Hz. Both the Celsius and the Fahreheit 
//! temperatures are calcuated.
//
//*****************************************************************************

uint32_t ui32ADC0Value;
volatile uint32_t ui32VoltageValue;
long count = 0;

void
PortFunctionInit(void)
{
		// configure the system clock to be 40MHz
		SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);    
		
		// Enable Peripheral Clocks 
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
		
		SysCtlDelay(2);	//insert a few cycles after enabling the peripheral to allow the clock to be fully activated.

    // Enable pin PE3 for ADC AIN0
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
}

//ADC0 initializaiton
void ADC0_Init(void)
{
		ADCSequenceDisable(ADC0_BASE, 3); //disable ADC0 before the configuration is complete
		ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0); // will use ADC0, SS3, processor-trigger, priority 0
		
		//ADC0 SS3 Step 0, sample from Pin PE3, completion of this step will set RIS, last sample of the sequence
		ADCSequenceStepConfigure(ADC0_BASE, 3,0,ADC_CTL_CH0|ADC_CTL_IE|ADC_CTL_END); 
	
		IntPrioritySet(INT_ADC0SS3, 0x00);  	 // configure ADC0 SS3 interrupt priority as 0
		IntEnable(INT_ADC0SS3);    				// enable interrupt 31 in NVIC (ADC0 SS3)
		ADCIntEnableEx(ADC0_BASE, ADC_INT_SS3);      // arm interrupt of ADC0 SS3
	
		ADCSequenceEnable(ADC0_BASE, 3); //enable ADC0
}

//interrupt handler for ADC0
void ADC0_Handler(void)
{
		ADCIntClear(ADC0_BASE, 3); // Initialize ADC0 SS3 Interrupt
		ADCSequenceDataGet(ADC0_BASE, 3, &ui32ADC0Value); // Get Data from ADC Sequence SS3
		ui32VoltageValue = (ui32ADC0Value/(4095/3300.0)) + 0.5;//Determine Voltage Potential in mV with VREFP = 3.3V,VREFN = 0V
		count++; // Increment Total Number of Samples Measured
}

// Timer Initialization
void Timer0A_Init(unsigned long period)
{   
	volatile uint32_t ui32Loop; 
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0; // activate timer0
  ui32Loop = SYSCTL_RCGC1_R;				// Do a dummy read to insert a few cycles after enabling the peripheral.
  TIMER0_CTL_R &= ~0x00000001;     // disable timer0A during setup
  TIMER0_CFG_R = 0x00000000;       // configure for 32-bit timer mode
  TIMER0_TAMR_R = 0x00000002;      // configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = period-1;       // reload value
	NVIC_PRI4_R &= ~0xE0000000; 	 // configure Timer0A interrupt priority as 0
  NVIC_EN0_R |= 0x00080000;     // enable interrupt 19 in NVIC (Timer0A)
	TIMER0_IMR_R |= 0x00000001;      // arm timeout interrupt
  TIMER0_CTL_R |= 0x00000001;      // enable timer0A
}

//interrupt handler for Timer0A
void Timer0A_Handler(void)
{
		// acknowledge flag for Timer0A
		TIMER0_ICR_R |= 0x00000001;
		ADCProcessorTrigger(ADC0_BASE, 3); // Generate a Proccessor Trigger for ADC Sequence SS3
}

int main(void)
{
		PortFunctionInit(); // Initialize Ports
		ADC0_Init(); // Initialize ADC0 Interrupt
		Timer0A_Init(SysCtlClockGet() / 1000); // Initialize Timer with Period of 1ms
		IntMasterEnable(); // globally enable interrupt
		ADCProcessorTrigger(ADC0_BASE, 3); // Generate a Proccessor Trigger for ADC Sequence SS3
	
		while(1)
		{
			
		}
}
