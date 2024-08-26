/*
 * Real Time Clock.c
 *
 *  Created on: JUL 4, 2024
 *  Author: Abdallah AbdelMomen
 */

#include "STD_MACROS.h"
#include "STD_TYPES.h"

#include "../MCAL_Layer/TIMER/TIMER_interface.h"
#include "../MCAL_Layer/DIO/DIO_interface.h"
#include "../MCAL_Layer/DIO/DIO_private.h"
#include "../MCAL_Layer/GIE/GIE_interface.h"
#include "../MCAL_Layer/ADC/ADC_interface.h"

#include "../HAL_Layer/BCD_TO_SSD/BCD_To_SSD_interface.h"
#include "../HAL_Layer/CLCD/CLCD_interface.h"
#include "../HAL_Layer/KPD/KPD_interface.h"
#include "../HAL_Layer/LM35/LM35_interface.h"
#include "../HAL_Layer/BUZZ/BUZ_interface.h"

#define F_CPU 8000000UL
#include <util/delay.h>

#define mode_12Hours '1'
#define mode_24Hours '2'

// Global variables
volatile u8  Mode_Flag = 1, Alarm_Flag = 0 , Alarm_EN = 0;
volatile u8 Hours_Alarm, Minutes_Alarm;

volatile u8  Hour = 12, Minute = 0, Second = 0;
volatile u8 press, day_time, mode = mode_12Hours;


/* SSD Configuration */
SSD_config seven_seg0 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN0,SSD_LOW_NIBBLE};
SSD_config seven_seg1 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN1,SSD_LOW_NIBBLE };
SSD_config seven_seg2 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN2,SSD_LOW_NIBBLE };
SSD_config seven_seg3 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN3,SSD_LOW_NIBBLE };
SSD_config seven_seg4 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN4,SSD_LOW_NIBBLE };
SSD_config seven_seg5 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN5,SSD_LOW_NIBBLE };

/* LM35 Configuration */
LM35_Config LM350 = {ADC_ADC0_ADC1, 5, ADC_RES_10_BIT};

/* BUZZER Configuration */
BUZ_config BUZ0 = {DIO_PORTB, DIO_PIN4, ACTIVE_HIGH};

void Set_Time(void);
void Select_Mode(void);
void Set_Alarm(void);
void Get_Temp(void);
void ISR_TIMER2_OVF_MODE(void);

//======================================================================================================================================//

void main(void)
{
	/* Configure AM/PM indicator pin as output */
	DIO_enumSetPinDir(DIO_PORTA, DIO_PIN3, DIO_PIN_OUTPUT);
	DIO_enumWritePinVal(DIO_PORTA, DIO_PIN3, DIO_HIGH);

	// Set callback function for TIMER2 overflow interrup
	TIMER_u8SetCallBack(ISR_TIMER2_OVF_MODE, TIMER2_OVF_VECTOR_ID);

	/* Initialize Keypad on PORTD */
	KPD_vInit();

	/* Initialize CLCD on High Nibble for PORTA */
	CLCD_vInit();

	/* Enable Global Interrupts */
	GIE_vEnable();

	/* Initialize BUZ on PIN4 for PORTB */
	BUZ_vInit(BUZ0);

	/* Initialize seven-segment displays */
	SSD_vInitDataPort(seven_seg0);
	SSD_vInitDataPort(seven_seg1);
	SSD_vInitDataPort(seven_seg2);
	SSD_vInitDataPort(seven_seg3);
	SSD_vInitDataPort(seven_seg4);
	SSD_vInitDataPort(seven_seg5);

	/*
	 * Initialize TIMER2 with external clock at 32.768 KHz
	 * Using division factor 128 to achieve 1 second intervals
	 */
	TIMER2_vInit();

	/* Initialize ADC for temperature sensor */
	ADC_vInit();

	CLCD_vSendString("Press ON to");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("Show Option");
	while(1)
	{
		press = KPD_u8GetPressed();
		if (press == 'A')
		{
			// Display menu options on CLCD
			CLCD_vClearScreen();
			CLCD_vSendString("1 - Set time");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("2 - Select mode");
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("3 - Set Alarm");
			CLCD_vSetPosition(4, 1);
			CLCD_vSendString("4 - Weather");
			CLCD_vSetPosition(4, 15);
			CLCD_vSendString("X:EXIT");

			do
			{
				press = KPD_u8GetPressed();
				if (press != NOTPRESSED)
				{
					// Perform actions based on the pressed key
					switch(press)
					{
					case '1':
						Set_Time();
						break;

					case '2':
						Select_Mode();
						break;

					case '3' :
						Set_Alarm();
						break;

					case '4':
						Get_Temp();
						break;

					case '*' :
						CLCD_vClearScreen();
						CLCD_vSendString("Press ON to");
						CLCD_vSetPosition(2, 1);
						CLCD_vSendString("Show Option");
						break;

					default:
						// Invalid choice handling
						CLCD_vClearScreen();
						CLCD_vSendString("wrong choice");
						_delay_ms(500);
						CLCD_vClearScreen();
						CLCD_vSendString("1 - set time");
						CLCD_vSetPosition(2,1);
						CLCD_vSendString("2 - select mode");
						CLCD_vSetPosition(3, 1);
						CLCD_vSendString("3 - Set Alarm");
						CLCD_vSetPosition(4, 1);
						CLCD_vSendString("4 - Weather");
						CLCD_vSetPosition(4, 15);
						CLCD_vSendString("X:EXIT");
						break;
					}
				}
			}while (press != '*' );

		}
		/* Display time on seven-segment displays using POV theorem */

		// Display seconds (units)
		SSD_vDisable(seven_seg5);
		SSD_vEnable(seven_seg0);
		SSD_vSendNumber(seven_seg0, Second%10);
		_delay_ms(5);

		// Display seconds (tens)
		SSD_vDisable(seven_seg0);
		SSD_vEnable(seven_seg1);
		SSD_vSendNumber(seven_seg1, Second/10);
		_delay_ms(5);

		// Display minutes (units)
		SSD_vDisable(seven_seg1);
		SSD_vEnable(seven_seg2);
		SSD_vSendNumber(seven_seg2, Minute%10);
		_delay_ms(5);

		// Display minutes (tens)
		SSD_vDisable(seven_seg2);
		SSD_vEnable(seven_seg3);
		SSD_vSendNumber(seven_seg3, Minute/10);
		_delay_ms(5);

		// Display hours (units)
		SSD_vDisable(seven_seg3);
		SSD_vEnable(seven_seg4);
		SSD_vSendNumber(seven_seg4, Hour %10);
		_delay_ms(5);

		// Display hours (tens)
		SSD_vDisable(seven_seg4);
		SSD_vEnable(seven_seg5);
		SSD_vSendNumber(seven_seg5, Hour /10);
		_delay_ms(5);

		if (Hour == Hours_Alarm && Minute == Minutes_Alarm && Second == 0 && Alarm_EN == 1)
		{
			CLCD_vClearScreen();
			CLCD_vSetPosition(1, 8);
			CLCD_vSendString("Alarm");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("Press any key");
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("to close");

			u8 press = NOTPRESSED;
			while(1)
			{
				press = KPD_u8GetPressed();
				if (press != NOTPRESSED)
				{
					break;
				}
				/* Display time on seven-segment displays using POV theorem */

				// Display seconds (units)
				SSD_vDisable(seven_seg5);
				SSD_vEnable(seven_seg0);
				SSD_vSendNumber(seven_seg0, Second%10);
				_delay_ms(5);

				// Display seconds (tens)
				SSD_vDisable(seven_seg0);
				SSD_vEnable(seven_seg1);
				SSD_vSendNumber(seven_seg1, Second/10);
				_delay_ms(5);

				// Display minutes (units)
				SSD_vDisable(seven_seg1);
				SSD_vEnable(seven_seg2);
				SSD_vSendNumber(seven_seg2, Minute%10);
				_delay_ms(5);

				// Display minutes (tens)
				SSD_vDisable(seven_seg2);
				SSD_vEnable(seven_seg3);
				SSD_vSendNumber(seven_seg3, Minute/10);
				_delay_ms(5);

				// Display hours (units)
				SSD_vDisable(seven_seg3);
				SSD_vEnable(seven_seg4);
				SSD_vSendNumber(seven_seg4, Hour %10);
				_delay_ms(5);

				// Display hours (tens)
				SSD_vDisable(seven_seg4);
				SSD_vEnable(seven_seg5);
				SSD_vSendNumber(seven_seg5, Hour /10);
				_delay_ms(5);

				BUZ_vTog(BUZ0);
				_delay_ms(10);
			}
			BUZ_vOff(BUZ0);
			CLCD_vClearScreen();
			CLCD_vSendString("1 - set time");
			CLCD_vSetPosition(2,1);
			CLCD_vSendString("2 - select mode");
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("3 - Set Alarm");
			CLCD_vSetPosition(4, 1);
			CLCD_vSendString("4 - Get Temp");
		}
	}
}

//======================================================================================================================================//

// Function to set time
void Set_Time()
{
	CLCD_vClearScreen();
	CLCD_vSendString("1 - 12-hour time");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("2 - 24-hour time");
	/* Select time mode: 12 Hours or 24 Hours */
	do
	{
		mode = KPD_u8GetPressed();

		if (mode != NOTPRESSED && (mode != mode_12Hours && mode != mode_24Hours))
		{
			// Handle wrong input
			CLCD_vClearScreen();
			CLCD_vSendString("wrong choise");
			_delay_ms(500);
			CLCD_vClearScreen();
			CLCD_vSendString("1 - 12-hour time");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("2 - 24-hour time");
		}
	}while (mode == NOTPRESSED || (mode != mode_12Hours && mode != mode_24Hours));

	// Set AM or PM flag based on selected mode
	if (mode ==  mode_12Hours)
	{
		Mode_Flag = 1;
	}
	else if (mode == mode_24Hours)
	{
		Mode_Flag = 0;
	}
	else
	{

	}

	CLCD_vClearScreen();

	// Display the current time on the CLCD
	CLCD_vSendData(Hour /10 +48);
	CLCD_vSendData(Hour %10 +48);
	CLCD_vSendString(" : ");
	CLCD_vSendData(Minute/10 + 48);
	CLCD_vSendData(Minute%10 + 48);
	CLCD_vSendString(" : ");
	CLCD_vSendData(Second/10 + 48);
	CLCD_vSendData(Second%10 + 48);
	CLCD_vSendData(' ');

	// Display AM/PM if in 12-hour mode
	if (mode == '1')
	{
		if (READ_BIT(PORTA_REG, DIO_PIN3) == 1)
		{
			CLCD_vSendString("AM");
		}
		else
		{
			CLCD_vSendString("PM");
		}
	}

	/* Change Time */
	CLCD_vSendCommand(CLCD_DISPLAYON_CURSORON);
	CLCD_vSetPosition(1,1);

	/* Set hours */
	// First Digit
	do
	{
		press = KPD_u8GetPressed();
	} while (press == NOTPRESSED);
	CLCD_vSetPosition(1,1);
	CLCD_vSendData( press );
	Hour = (press - 48) * 10;

	// Second Digit
	do
	{
		press = KPD_u8GetPressed();
	} while (press == NOTPRESSED);
	CLCD_vSendData(press);

	// Calculate total hours
	Hour += (press - 48) ;

	/* Check mode to change hour limit
	 * ==> if mode is 24 hours --> hour is less than 24
	 * ==> if mode is 12 hours --> hour is less than 12
	 */
	if (mode == mode_12Hours)
	{
		if (Hour > 12 || Hour == 0)
		{
			Hour = 12;
			CLCD_vSetPosition(1, 1);
			CLCD_vSendData(Hour /10 +48);
			CLCD_vSendData(Hour %10 +48);
		}
		else
		{

		}
	}
	else if (mode == mode_24Hours)
	{
		if (Hour  > 23)
		{
			Hour = 0;
			CLCD_vSetPosition(1, 1);
			CLCD_vSendData(Hour /10 +48);
			CLCD_vSendData(Hour %10 +48);
		}
		else
		{

		}
	}
	else
	{

	}
	/* Set minutes and seconds similar to hours */

	// Set minutes
	CLCD_vSetPosition(1,6);
	do
	{
		press = KPD_u8GetPressed();
	} while (press == NOTPRESSED);
	CLCD_vSendData(press);
	Minute = (press - 48) * 10;

	do
	{
		press = KPD_u8GetPressed();

	} while (press == NOTPRESSED);
	CLCD_vSendData(press);

	Minute += (press - 48);


	if (Minute > 59)
	{
		Minute = 0;
		CLCD_vSetPosition(1,6);
		CLCD_vSendData(Minute/10 + 48);
		CLCD_vSendData(Minute%10 + 48);
	}

	// Set seconds
	CLCD_vSetPosition(1,11);
	do
	{
		press = KPD_u8GetPressed();

	} while (press == NOTPRESSED);
	CLCD_vSendData(press);
	Second = (press - 48) * 10;

	do
	{
		press = KPD_u8GetPressed();

	} while (press == NOTPRESSED);
	CLCD_vSendData(press);

	Second += (press - 48);

	if (Second > 59)
	{
		Second = 0;
		CLCD_vSetPosition(1,11);
		CLCD_vSendData(Second/10 + 48);
		CLCD_vSendData(Second%10 + 48);
	}

	/* Select AM or PM if mode is 12-hour */
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1 : AM    2 : PM");

	if (mode == mode_12Hours)
	{
		do
		{
			day_time = KPD_u8GetPressed();

			if (day_time != NOTPRESSED && (day_time != '1' && day_time != '2'))
			{
				CLCD_vClearScreen();
				CLCD_vSendString("Wrong Choise");
				_delay_ms(500);
				CLCD_vClearScreen();
				CLCD_vSetPosition(1, 1);
				CLCD_vSendData(Hour /10 +48);
				CLCD_vSendData(Hour %10 +48);
				CLCD_vSendString(" : ");
				CLCD_vSendData(Minute/10 + 48);
				CLCD_vSendData(Minute%10 + 48);
				CLCD_vSendString(" : ");
				CLCD_vSendData(Second/10 + 48);
				CLCD_vSendData(Second%10 + 48);

				if (READ_BIT(PORTA_REG, DIO_PIN3) == 1)
				{
					CLCD_vSendString(" AM");
				}
				else
				{
					CLCD_vSendString(" PM");
				}
				CLCD_vSetPosition(2, 1);
				CLCD_vSendString("1 : AM    2 : PM");
			}
			else
			{

			}
		}while (day_time == NOTPRESSED || (day_time != '1' && day_time != '2'));

	}
	else
	{

	}
	CLCD_vSendCommand(CLCD_DISPLAYON_CURSOROFF);
	CLCD_vClearScreen();
	CLCD_vSendString("1 - set time");
	CLCD_vSetPosition(2,1);
	CLCD_vSendString("2 - select mode");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("3 - Set Alarm");
	CLCD_vSetPosition(4, 1);
	CLCD_vSendString("4 - Weather");
	CLCD_vSetPosition(4, 15);
	CLCD_vSendString("X:EXIT");
}

//======================================================================================================================================//

// Function to select time mode (12-hour or 24-hour)
void Select_Mode()
{

	CLCD_vClearScreen();
	CLCD_vSendString("1 - 12-hour time");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("2 - 24-hour time");

	do
	{

		mode = KPD_u8GetPressed();
		if (mode == '1')
		{
			if (Mode_Flag == 0)
			{
				if (Hour > 12 )
				{
					Hour -= 12;
				}
				else if (Hour == 0)
				{
					Hour += 12;
				}
				else
				{

				}
				Mode_Flag = 1;
			}
			else
			{

			}
		}
		else if (mode == '2')
		{
			if (Mode_Flag == 1)
			{
				if (Hour == 12 && READ_BIT(PORTA_REG, DIO_PIN3) == 1)
				{
					Hour = 0;
				}
				else if(Hour == 12 && READ_BIT(PORTA_REG, DIO_PIN3 == 0))
				{

				}
				else if (READ_BIT(PORTA_REG, DIO_PIN3) == 0)
				{
					Hour += 12;
				}
				else
				{

				}
				Mode_Flag = 0;
			}
			else
			{

			}
		}
		else if (mode != NOTPRESSED && (mode != '1' && mode != '2'))
		{
			CLCD_vClearScreen();
			CLCD_vSendString("Wrong choise");
			_delay_ms(500);
			CLCD_vClearScreen();
			CLCD_vSendString("1 - 12-hour time");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("2 - 24-hour time");
		}

	}while (mode == NOTPRESSED || (mode != '1' && mode != '2'));

	CLCD_vClearScreen();
	CLCD_vClearScreen();
	CLCD_vSendString("1 - set time");
	CLCD_vSetPosition(2,1);
	CLCD_vSendString("2 - select mode");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("3 - Set Alarm");
	CLCD_vSetPosition(4, 1);
	CLCD_vSendString("4 - Weather");
	CLCD_vSetPosition(4, 15);
	CLCD_vSendString("X:EXIT");
}

//======================================================================================================================================//

void Set_Alarm()
{
	CLCD_vClearScreen();
	CLCD_vSendString("ALARM");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1 - Enable");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("2 - Disable");

	do
	{
		press = KPD_u8GetPressed();
		if (press != NOTPRESSED &&press != '1' && press != '2')
		{
			CLCD_vClearScreen();
			CLCD_vSendString("Wrong Choise");
			_delay_ms(500);
			CLCD_vClearScreen();
			CLCD_vSendString("ALARM");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("1 - Enable");
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("2 - Disable");
		}
	}while(press == NOTPRESSED || (press != '1' && press != '2'));
	/* Check if chosen alarm is enable or disaple */
	/* if user choose alarm is enable */
	if (press == '1')
	{
		switch(Alarm_EN)
		{
		case 0:
			CLCD_vClearScreen();
			CLCD_vSendString("Set Aalrm :");
			CLCD_vSetPosition(2, 6);
			CLCD_vSendString("12 : 00");
			if (mode == mode_12Hours)
			{
				CLCD_vSetPosition(2, 14);
				CLCD_vSendString("AM");
			}
			break;
		case 1 :
			CLCD_vClearScreen();
			CLCD_vSendString("Set Aalrm :");
			CLCD_vSetPosition(2, 6);
			CLCD_vSendData((Hours_Alarm / 10 ) + 48);
			CLCD_vSendData((Hours_Alarm % 10 ) + 48);
			CLCD_vSendString(" : ");
			CLCD_vSendData((Minutes_Alarm / 10 ) + 48);
			CLCD_vSendData((Minutes_Alarm % 10 ) + 48);
		}

		CLCD_vSetPosition(2, 6);

		CLCD_vSendCommand(CLCD_DISPLAYON_CURSORON);

		/* Set Hour Alarm */
		do
		{
			press = KPD_u8GetPressed();
		}while (press == NOTPRESSED);

		CLCD_vSendData(press);

		Hours_Alarm = (press - 48) * 10;
		do
		{
			press = KPD_u8GetPressed();
		}while(press == NOTPRESSED);

		CLCD_vSendData(press);
		Hours_Alarm += (press - 48);

		//Check if it valid or not
		switch(mode)
		{
		case mode_12Hours:
			if (Hours_Alarm > 12 && Hours_Alarm < 24 )
			{
				Hours_Alarm -= 12;
				CLCD_vSetPosition(2, 6);
				CLCD_vSendData((Hours_Alarm / 10) + 48);
				CLCD_vSendData((Hours_Alarm % 10) + 48);
			}
			else if (Hours_Alarm > 23)
			{
				Hours_Alarm = 12;
				CLCD_vSetPosition(2, 6);
				CLCD_vSendData((Hours_Alarm / 10) + 48);
				CLCD_vSendData((Hours_Alarm % 10) + 48);
			}
			else
			{

			}
			break;

		case mode_24Hours:
			if (Hours_Alarm > 23)
			{
				Hours_Alarm = 12;
				CLCD_vSetPosition(2, 6);
				CLCD_vSendData((Hours_Alarm / 10) + 48);
				CLCD_vSendData((Hours_Alarm % 10) + 48);
			}
			else
			{

			}
			break;

		default:
			break;
		}

		CLCD_vSetPosition(2, 11);
		/* Set Minute Alarm */
		do
		{
			press = KPD_u8GetPressed();
		}while (press == NOTPRESSED);

		CLCD_vSendData(press);

		Minutes_Alarm = (press - 48) * 10;
		do
		{
			press = KPD_u8GetPressed();
		}while(press == NOTPRESSED);

		CLCD_vSendData(press);
		Minutes_Alarm += (press - 48);

		if (Minutes_Alarm > 12)
		{
			Minutes_Alarm = 0;
			CLCD_vSetPosition(2, 11);
			CLCD_vSendData((Minutes_Alarm / 10) + 48);
			CLCD_vSendData((Minutes_Alarm % 10) + 48);
		}
		/* Make Alarm Status : Alarm Enable  */
		Alarm_EN = 1 ;
		//=====================================
	}
	/* if user choose alarm is disaple */
	else if (press == '2')
	{
		/* Make Alarm Status : Alarm Disaple  */
		Hours_Alarm = NOTPRESSED;
		Alarm_EN = 0;
		//=====================================
	}
	else
	{

	}

	_delay_ms(200);
	CLCD_vSendCommand(CLCD_DISPLAYON_CURSOROFF);
	CLCD_vClearScreen();
	CLCD_vSendString("1 - set time");
	CLCD_vSetPosition(2,1);
	CLCD_vSendString("2 - select mode");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("3 - Set Alarm");
	CLCD_vSetPosition(4, 1);
	CLCD_vSendString("4 - Weather");
	CLCD_vSetPosition(4, 15);
	CLCD_vSendString("X:EXIT");
}

//======================================================================================================================================//

void Get_Temp()
{
	volatile u8  TEMP_val, press_button = NOTPRESSED;
	volatile u16 TEMP_volt ;
	CLCD_vClearScreen();
	CLCD_vSendString("Press any key");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("To Exit");
	while (1)
	{
		if (press_button != NOTPRESSED)
		{
			CLCD_vClearScreen();
			CLCD_vSendString("1 - set time");
			CLCD_vSetPosition(2,1);
			CLCD_vSendString("2 - select mode");
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("3 - Set Alarm");
			CLCD_vSetPosition(4, 1);
			CLCD_vSendString("4 - Weather");
			CLCD_vSetPosition(4, 15);
			CLCD_vSendString("X:EXIT");
			break;
		}
		LM35_u8GetAnalogSignal(&LM350, &TEMP_volt);
		CLCD_vSetPosition(4, 10);
		CLCD_vSendString("TEMP : ");
		if(TEMP_volt >= 1000)
		{
			TEMP_val = (TEMP_volt - 1000) / 10;
			if (TEMP_val < 10)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData(TEMP_val + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
				CLCD_vSendData(' ');
			}
			else if (TEMP_val < 100)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData((TEMP_val /10) + 48);
				CLCD_vSendData((TEMP_val % 10) + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
			}
			else if (TEMP_val < 1000)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData((TEMP_val / 100) + 48);
				CLCD_vSendData(((TEMP_val / 10) % 10) + 48);
				CLCD_vSendData((TEMP_val % 10) + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
			}
		}
		else
		{
			TEMP_val = (1000 - TEMP_volt ) / 10;
			if (TEMP_val < 10)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData('-');
				CLCD_vSendData(TEMP_val + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
				CLCD_vSendData(' ');
			}
			else if (TEMP_val < 100)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData('-');
				CLCD_vSendData((TEMP_val/10) + 48);
				CLCD_vSendData((TEMP_val % 10) + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
			}
		}
	}
}

//======================================================================================================================================//

/* ISR for timer OverFlow */
void ISR_TIMER2_OVF_MODE()
{
	Second++;

	if (Second == 60)
	{
		Second = 0;
		Minute++;
	}
	if (Minute == 60)
	{
		Minute=0;
		Hour++;
	}
	if (mode == '1')
	{
		if (Hour== 13)
		{
			Hour= 1;
		}
		if (Hour == 12 && Minute == 0 && Second == 0 )
		{
			TOG_BIT(PORTA_REG, DIO_PIN3);
		}
	}
	if (mode == '2')
	{
		if (Hour == 24)
		{
			Hour = 0;
		}
		else
		{

		}
		if (Hour < 12)
		{
			SET_BIT(PORTA_REG, DIO_PIN3);
		}
		else if (Hour == 12 && Minute == 0 && Second == 0)
		{
			CLR_BIT(PORTA_REG, DIO_PIN3);
		}
		else if (Hour > 12)
		{
			CLR_BIT(PORTA_REG, DIO_PIN3);
		}
		else
		{

		}
	}
}
