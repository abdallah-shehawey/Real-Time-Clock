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

#define Time_Mode12Hours '1'
#define Time_Mode24Hours '2'

#define Alarm_DIS 0
#define Alarm_EN  1

// Global variables
volatile u8 Alarm_DayTime = 0, Alarm_Status = Alarm_DIS;
volatile u8 Alarm_Hour, Alarm_Minutes;

volatile u8 Time_ModeFlag = 1, Time_Hour = 12, Time_Minute = 0, Time_Second = 0;
volatile u8 KPD_Press, Time_DayFlag, Time_Mode = Time_Mode12Hours;

/* SSD Configuration */
SSD_config seven_seg0 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN0, SSD_LOW_NIBBLE};
SSD_config seven_seg1 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN1, SSD_LOW_NIBBLE};
SSD_config seven_seg2 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN2, SSD_LOW_NIBBLE};
SSD_config seven_seg3 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN3, SSD_LOW_NIBBLE};
SSD_config seven_seg4 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN4, SSD_LOW_NIBBLE};
SSD_config seven_seg5 = {SSD_COMMON_ANODE, SSD_PORTB, SSD_PORTC, DIO_PIN5, SSD_LOW_NIBBLE};

/* LM35 Configuration */
LM35_Config LM350 = {ADC_ADC0_ADC1, 5, ADC_RES_10_BIT};

/* BUZZER Configuration */
BUZ_config BUZ0 = {DIO_PORTB, DIO_PIN4, ACTIVE_HIGH};

/* Functions Proto Type */
void Set_Time                        (void);
void Select_Mode                  (void);
void Set_Alarm                      (void);
void Get_Temp                      (void);
void ISR_TIMER2_OVF_MODE(void);

//======================================================================================================================================//

void main(void)
{
	/* Configure AM/PM indicator pin as output */
	DIO_enumSetPinDir(DIO_PORTA, DIO_PIN3, DIO_PIN_OUTPUT);
	DIO_enumWritePinVal(DIO_PORTA, DIO_PIN3, DIO_HIGH);
	/* To sure that ISR execute each 1 sec */
	DIO_enumSetPinDir(DIO_PORTA, DIO_PIN2, DIO_PIN_OUTPUT);

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
	while (1)
	{
		KPD_Press = KPD_u8GetPressed();
		if (KPD_Press == 'A')
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
				KPD_Press = KPD_u8GetPressed();
				if (KPD_Press != NOTPRESSED)
				{
					// Perform actions based on the pressed key
					switch (KPD_Press)
					{
					case '1':
						Set_Time();
						break;

					case '2':
						Select_Mode();
						break;

					case '3':
						Set_Alarm();
						break;

					case '4':
						Get_Temp();
						break;

					case '*':
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
						CLCD_vSetPosition(2, 1);
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
			} while (KPD_Press != '*');
		}
		/* Display time on seven-segment displays using POV theorem */

		// Display seconds (units)
		SSD_vDisable(seven_seg5);
		SSD_vEnable(seven_seg0);
		SSD_vSendNumber(seven_seg0, Time_Second % 10);
		_delay_ms(5);

		// Display seconds (tens)
		SSD_vDisable(seven_seg0);
		SSD_vEnable(seven_seg1);
		SSD_vSendNumber(seven_seg1, Time_Second / 10);
		_delay_ms(5);

		// Display minutes (units)
		SSD_vDisable(seven_seg1);
		SSD_vEnable(seven_seg2);
		SSD_vSendNumber(seven_seg2, Time_Minute % 10);
		_delay_ms(5);

		// Display minutes (tens)
		SSD_vDisable(seven_seg2);
		SSD_vEnable(seven_seg3);
		SSD_vSendNumber(seven_seg3, Time_Minute / 10);
		_delay_ms(5);

		// Display hours (units)
		SSD_vDisable(seven_seg3);
		SSD_vEnable(seven_seg4);
		SSD_vSendNumber(seven_seg4, Time_Hour % 10);
		_delay_ms(5);

		// Display hours (tens)
		SSD_vDisable(seven_seg4);
		SSD_vEnable(seven_seg5);
		SSD_vSendNumber(seven_seg5, Time_Hour / 10);
		_delay_ms(5);

		/* When Alarm is Ring */
		if (Time_Hour == Alarm_Hour && Time_Minute == Alarm_Minutes && Time_Second == 0 && Alarm_Status == Alarm_EN && READ_BIT(PORTA_REG, DIO_PIN3) == Alarm_DayTime)
		{
			CLCD_vClearScreen();
			CLCD_vSetPosition(1, 8);
			CLCD_vSendString("Alarm");

			CLCD_vSetPosition(2, 6);
			CLCD_vSendData((Alarm_Hour / 10) + 48);
			CLCD_vSendData((Alarm_Hour % 10) + 48);
			CLCD_vSendString(" : ");
			CLCD_vSendData((Alarm_Minutes / 10) + 48);
			CLCD_vSendData((Alarm_Minutes % 10) + 48);
			switch (Alarm_DayTime)
			{
			case 1:
				CLCD_vSendString(" AM");
				break;
			case 0:
				CLCD_vSendString(" PM");
				break;
			}
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("Press any key");
			CLCD_vSetPosition(4, 1);
			CLCD_vSendString("to close");

			while (1)
			{
				KPD_Press = KPD_u8GetPressed();
				if (KPD_Press != NOTPRESSED)
				{
					break;
				}
				/* Display time on seven-segment displays using POV theorem */

				// Display seconds (units)
				SSD_vDisable(seven_seg5);
				SSD_vEnable(seven_seg0);
				SSD_vSendNumber(seven_seg0, Time_Second % 10);
				_delay_ms(5);

				// Display seconds (tens)
				SSD_vDisable(seven_seg0);
				SSD_vEnable(seven_seg1);
				SSD_vSendNumber(seven_seg1, Time_Second / 10);
				_delay_ms(5);

				// Display minutes (units)
				SSD_vDisable(seven_seg1);
				SSD_vEnable(seven_seg2);
				SSD_vSendNumber(seven_seg2, Time_Minute % 10);
				_delay_ms(5);

				// Display minutes (tens)
				SSD_vDisable(seven_seg2);
				SSD_vEnable(seven_seg3);
				SSD_vSendNumber(seven_seg3, Time_Minute / 10);
				_delay_ms(5);

				// Display hours (units)
				SSD_vDisable(seven_seg3);
				SSD_vEnable(seven_seg4);
				SSD_vSendNumber(seven_seg4, Time_Hour % 10);
				_delay_ms(5);

				// Display hours (tens)
				SSD_vDisable(seven_seg4);
				SSD_vEnable(seven_seg5);
				SSD_vSendNumber(seven_seg5, Time_Hour / 10);
				_delay_ms(5);

				BUZ_vTog(BUZ0);
				_delay_ms(10);
			}
			BUZ_vOff(BUZ0);
			CLCD_vClearScreen();
			CLCD_vSendString("Press ON to");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("Show Option");
		}
		/*___________________________________________________________________________________________________________________*/
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
		/*___________________________________________________________________________________________________________________*/

		Time_Mode = KPD_u8GetPressed();

		if (Time_Mode != NOTPRESSED && (Time_Mode != Time_Mode12Hours && Time_Mode != Time_Mode24Hours))
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
	} while (Time_Mode == NOTPRESSED || (Time_Mode != Time_Mode12Hours && Time_Mode != Time_Mode24Hours));

	/*___________________________________________________________________________________________________________________*/

	// Set AM or PM flag based on selected mode
	if (Time_Mode == Time_Mode12Hours)
	{
		Time_ModeFlag = 1;
	}
	else if (Time_Mode == Time_Mode24Hours)
	{
		Time_ModeFlag = 0;
	}
	else
	{

	}

	/*___________________________________________________________________________________________________________________*/

	CLCD_vClearScreen();

	/*___________________________________________________________________________________________________________________*/

	// Display the current time on the CLCD
	CLCD_vSendData(Time_Hour / 10 + 48);
	CLCD_vSendData(Time_Hour % 10 + 48);
	CLCD_vSendString(" : ");
	CLCD_vSendData(Time_Minute / 10 + 48);
	CLCD_vSendData(Time_Minute % 10 + 48);
	CLCD_vSendString(" : ");
	CLCD_vSendData(Time_Second / 10 + 48);
	CLCD_vSendData(Time_Second % 10 + 48);
	CLCD_vSendData(' ');

	// Display AM/PM if in 12-hour mode
	if (Time_Mode == Time_Mode12Hours)
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

	/*___________________________________________________________________________________________________________________*/

	/* Change Time */
	CLCD_vSendCommand(CLCD_DISPLAYON_CURSORON);
	CLCD_vSetPosition(1, 1);

	/* Set hours */
	// First Digit
	do
	{
		KPD_Press = KPD_u8GetPressed();
	} while (KPD_Press == NOTPRESSED);
	CLCD_vSetPosition(1, 1);
	CLCD_vSendData(KPD_Press);
	Time_Hour = (KPD_Press - 48) * 10;

	// Second Digit
	do
	{
		KPD_Press = KPD_u8GetPressed();
	} while (KPD_Press == NOTPRESSED);
	CLCD_vSendData(KPD_Press);

	// Calculate total hours
	Time_Hour += (KPD_Press - 48);

	/* Check mode to change hour limit
	 * ==> if mode is 24 hours --> hour is less than 24
	 * ==> if mode is 12 hours --> hour is less than 12
	 */

	/*___________________________________________________________________________________________________________________*/

	if (Time_Mode == Time_Mode12Hours)
	{
		if (Time_Hour > 12 || Time_Hour == 0)
		{
			Time_Hour = 12;
			CLCD_vSetPosition(1, 1);
			CLCD_vSendData(Time_Hour / 10 + 48);
			CLCD_vSendData(Time_Hour % 10 + 48);
		}
		else
		{
		}
	}
	else if (Time_Mode == Time_Mode24Hours)
	{
		if (Time_Hour > 23)
		{
			Time_Hour = 0;
			CLCD_vSetPosition(1, 1);
			CLCD_vSendData(Time_Hour / 10 + 48);
			CLCD_vSendData(Time_Hour % 10 + 48);
		}
		else
		{
		}
		if (Time_Hour == 12)
		{
			CLR_BIT(PORTA_REG, DIO_PIN3);
		}
		else
		{
		}
	}
	else
	{
	}

	/*___________________________________________________________________________________________________________________*/

	/* Set minutes and seconds similar to hours */

	// Set minutes
	CLCD_vSetPosition(1, 6);
	do
	{
		KPD_Press = KPD_u8GetPressed();
	} while (KPD_Press == NOTPRESSED);
	CLCD_vSendData(KPD_Press);
	Time_Minute = (KPD_Press - 48) * 10;

	do
	{
		KPD_Press = KPD_u8GetPressed();

	} while (KPD_Press == NOTPRESSED);
	CLCD_vSendData(KPD_Press);

	Time_Minute += (KPD_Press - 48);

	if (Time_Minute > 59)
	{
		Time_Minute = 0;
		CLCD_vSetPosition(1, 6);
		CLCD_vSendData(Time_Minute / 10 + 48);
		CLCD_vSendData(Time_Minute % 10 + 48);
	}

	/*___________________________________________________________________________________________________________________*/

	// Set seconds
	CLCD_vSetPosition(1, 11);
	do
	{
		KPD_Press = KPD_u8GetPressed();

	} while (KPD_Press == NOTPRESSED);
	CLCD_vSendData(KPD_Press);
	Time_Second = (KPD_Press - 48) * 10;

	do
	{
		KPD_Press = KPD_u8GetPressed();

	} while (KPD_Press == NOTPRESSED);
	CLCD_vSendData(KPD_Press);

	Time_Second += (KPD_Press - 48);

	if (Time_Second > 59)
	{
		Time_Second = 0;
		CLCD_vSetPosition(1, 11);
		CLCD_vSendData(Time_Second / 10 + 48);
		CLCD_vSendData(Time_Second % 10 + 48);
	}

	/*___________________________________________________________________________________________________________________*/

	//display AM or PM when mode is 12 Hour
	if (Time_Mode == Time_Mode12Hours)
	{
		/* Select AM or PM if mode is 12-hour */
		CLCD_vSetPosition(2, 1);
		CLCD_vSendString("1 : AM    2 : PM");
		do
		{
			Time_DayFlag = KPD_u8GetPressed();
			// Handle wrong input
			if (Time_DayFlag != NOTPRESSED && (Time_DayFlag != '1' && Time_DayFlag != '2'))
			{
				CLCD_vClearScreen();
				CLCD_vSendString("Wrong Choise");
				_delay_ms(500);
				CLCD_vClearScreen();
				CLCD_vSetPosition(1, 1);
				CLCD_vSendData(Time_Hour / 10 + 48);
				CLCD_vSendData(Time_Hour % 10 + 48);
				CLCD_vSendString(" : ");
				CLCD_vSendData(Time_Minute / 10 + 48);
				CLCD_vSendData(Time_Minute % 10 + 48);
				CLCD_vSendString(" : ");
				CLCD_vSendData(Time_Second / 10 + 48);
				CLCD_vSendData(Time_Second % 10 + 48);

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
		} while (Time_DayFlag == NOTPRESSED || (Time_DayFlag != '1' && Time_DayFlag != '2'));

		// if day time is AM
		if (Time_DayFlag == '1')
		{
			SET_BIT(PORTA_REG, DIO_PIN3);
		}
		// if day time is PM
		else if (Time_DayFlag == '2')
		{
			CLR_BIT(PORTA_REG, DIO_PIN3);
		}
		else
		{
		}
	}
	else
	{

	}
	//to make sure that the user enter correct our or not to Re enter Clock again
	CLCD_vClearScreen();
	CLCD_vSendString("Set Time ?");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1 : OK    2 : Cancel");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("Cancel to Set");
	CLCD_vSetPosition(4, 1);
	CLCD_vSendString("Time Again");
	do
	{
		KPD_Press = KPD_u8GetPressed();
		//when input invalid val
		if (KPD_Press != NOTPRESSED && (KPD_Press != '1' && KPD_Press != '2'))
		{
			CLCD_vClearScreen();
			CLCD_vSendString("Wrong Choise");
			_delay_ms(500);
			CLCD_vClearScreen();
			CLCD_vSendString("Set Time ?");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("1 : OK    2 : Cancel");
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("Cancel to Set");
			CLCD_vSetPosition(4, 1);
			CLCD_vSendString("Time Again");
		}
	} while (KPD_Press != '1' && KPD_Press != '2');

	//when enter correct clock
	if (KPD_Press == '1')
	{
		CLCD_vSendCommand(CLCD_DISPLAYON_CURSOROFF);
		CLCD_vClearScreen();
		CLCD_vSendString("1 - set time");
		CLCD_vSetPosition(2, 1);
		CLCD_vSendString("2 - select mode");
		CLCD_vSetPosition(3, 1);
		CLCD_vSendString("3 - Set Alarm");
		CLCD_vSetPosition(4, 1);
		CLCD_vSendString("4 - Weather");
		CLCD_vSetPosition(4, 15);
		CLCD_vSendString("X:EXIT");
	}
	//when enter incorrect clock
	else if (KPD_Press == '2')
	{
		Set_Time();
	}
}

//======================================================================================================================================//

// Function to select time mode (12-hour or 24-hour)
//it is created for Every possible scenario
void Select_Mode()
{

	CLCD_vClearScreen();
	CLCD_vSendString("1 - 12-hour time");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("2 - 24-hour time");
	//Get input from user
	do
	{
		Time_Mode = KPD_u8GetPressed();
		//if Select 12 hour mode
		if (Time_Mode == Time_Mode12Hours)
		{
			if (Time_ModeFlag == 0)
			{
				if (Time_Hour > 12)
				{
					Time_Hour -= 12;
				}
				else if (Time_Hour == 0 && READ_BIT(PORTA_REG,DIO_PIN3) == 1)
				{
					Time_Hour = 12;
				}
				else if (Time_Hour)
				{
					Time_Hour += 12;
				}
				else
				{
				}
				Time_ModeFlag = 1;
			}
			else
			{
			}
		}
		//if Select 24 hour mode
		else if (Time_Mode == Time_Mode24Hours)
		{
			if (Time_ModeFlag == 1)
			{
				if (Time_Hour == 12 && READ_BIT(PORTA_REG, DIO_PIN3) == 1)
				{
					Time_Hour = 0;
				}
				else if (Time_Hour == 12 && READ_BIT(PORTA_REG, DIO_PIN3 == 0))
				{
				}
				else if (READ_BIT(PORTA_REG, DIO_PIN3) == 0)
				{
					Time_Hour += 12;
				}
				else
				{
				}
				Time_ModeFlag = 0;
			}
			else
			{
			}
		}
		//when inter invalid choose
		else if (Time_Mode != NOTPRESSED && (Time_Mode != '1' && Time_Mode != '2'))
		{
			CLCD_vClearScreen();
			CLCD_vSendString("Wrong choise");
			_delay_ms(500);
			CLCD_vClearScreen();
			CLCD_vSendString("1 - 12-hour time");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("2 - 24-hour time");
		}

	} while (Time_Mode == NOTPRESSED || (Time_Mode != '1' && Time_Mode != '2'));

	//when choose any of them
	CLCD_vClearScreen();
	CLCD_vSendString("1 - set time");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("2 - select mode");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("3 - Set Alarm");
	CLCD_vSetPosition(4, 1);
	CLCD_vSendString("4 - Weather");
	CLCD_vSetPosition(4, 15);
	CLCD_vSendString("X:EXIT");
}

//======================================================================================================================================//

/* Fuction To set the time when Alarm is Ringing */
void Set_Alarm()
{
	CLCD_vClearScreen();
	CLCD_vSendString("ALARM");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1 - Enable");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("2 - Disable");

	/* Get pressed to choose if Alarm Statsu IS Enable or Disaple */
	do
	{
		KPD_Press = KPD_u8GetPressed();
		if (KPD_Press != NOTPRESSED && KPD_Press != '1' && KPD_Press != '2')
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
	} while (KPD_Press == NOTPRESSED || (KPD_Press != '1' && KPD_Press != '2'));

	/*___________________________________________________________________________________________________________________*/

	/* Check if chosen Alarm Status is Enable or Disaple */
	/* if user choose alarm is enable */
	if (KPD_Press == '1')
	{
		/* Check if Alarm was Enaple or Disaple */
		switch (Alarm_Status)
		{
		//if Alarm was Disaple display Alarm is 12 : 00
		case Alarm_DIS:
			CLCD_vClearScreen();
			CLCD_vSendString("Set Aalrm :");
			CLCD_vSetPosition(2, 6);
			CLCD_vSendString("12 : 00");
			//if Time_Mode is 12 Hours Mode display AM or PM
			if (Time_Mode == Time_Mode12Hours)
			{
				CLCD_vSetPosition(2, 14);
				CLCD_vSendString("AM");
			}
			break;
			//If Alarm was Enaple display Last Alarm
		case Alarm_EN:
			CLCD_vClearScreen();
			CLCD_vSendString("Set Aalrm :");
			CLCD_vSetPosition(2, 6);
			CLCD_vSendData((Alarm_Hour / 10) + 48);
			CLCD_vSendData((Alarm_Hour % 10) + 48);
			CLCD_vSendString(" : ");
			CLCD_vSendData((Alarm_Minutes / 10) + 48);
			CLCD_vSendData((Alarm_Minutes % 10) + 48);

			//if Time_Mode is 12 Hours Mode display AM or PM
			if (Time_Mode == Time_Mode12Hours)
			{
				switch (Alarm_DayTime)
				{
				case 1:
					CLCD_vSendString(" AM");
					break;
				case 0:
					CLCD_vSendString(" PM");
					break;
				}
			}
		}

		/*___________________________________________________________________________________________________________________*/

		CLCD_vSetPosition(2, 6);

		CLCD_vSendCommand(CLCD_DISPLAYON_CURSORON);

		/*___________________________________________________________________________________________________________________*/

		/* Set Alarm_Hour */
		do
		{
			KPD_Press = KPD_u8GetPressed();
		} while (KPD_Press == NOTPRESSED);

		CLCD_vSendData(KPD_Press);

		Alarm_Hour = (KPD_Press - 48) * 10;
		do
		{
			KPD_Press = KPD_u8GetPressed();
		} while (KPD_Press == NOTPRESSED);

		CLCD_vSendData(KPD_Press);
		Alarm_Hour += (KPD_Press - 48);

		/* Check if it valid or not */
		switch (Time_Mode)
		{
		// Check if it valid or not In case it is in 12 hour mode
		case Time_Mode12Hours:
			if (Alarm_Hour > 12 && Alarm_Hour < 24)
			{
				Alarm_Hour -= 12;
				CLCD_vSetPosition(2, 6);
				CLCD_vSendData((Alarm_Hour / 10) + 48);
				CLCD_vSendData((Alarm_Hour % 10) + 48);
			}
			else if (Alarm_Hour > 23)
			{
				Alarm_Hour = 12;
				CLCD_vSetPosition(2, 6);
				CLCD_vSendData((Alarm_Hour / 10) + 48);
				CLCD_vSendData((Alarm_Hour % 10) + 48);
			}
			else
			{
			}
			break;
			// Check if it valid or not In case it is in 24 hour mode
		case Time_Mode24Hours:
			if (Alarm_Hour > 23)
			{
				Alarm_Hour = 12;
				CLCD_vSetPosition(2, 6);
				CLCD_vSendData((Alarm_Hour / 10) + 48);
				CLCD_vSendData((Alarm_Hour % 10) + 48);
			}
			else
			{
			}
			break;

		default:
			break;
		}

		/*___________________________________________________________________________________________________________________*/

		CLCD_vSetPosition(2, 11);

		/* Set Alarm Minutes */
		do
		{
			KPD_Press = KPD_u8GetPressed();
		} while (KPD_Press == NOTPRESSED);

		CLCD_vSendData(KPD_Press);

		Alarm_Minutes = (KPD_Press - 48) * 10;
		do
		{
			KPD_Press = KPD_u8GetPressed();
		} while (KPD_Press == NOTPRESSED);

		CLCD_vSendData(KPD_Press);
		Alarm_Minutes += (KPD_Press - 48);

		//Check if it valid or not
		if (Alarm_Minutes > 59)
		{
			Alarm_Minutes = 0;
			CLCD_vSetPosition(2, 11);
			CLCD_vSendData((Alarm_Minutes / 10) + 48);
			CLCD_vSendData((Alarm_Minutes % 10) + 48);
		}

		/*___________________________________________________________________________________________________________________*/

		/* Check time mode to display AM / PM or not and Choose the time mode */
		//if time mode is 12 hours disply AM or PM and choose between them
		if (Time_Mode == Time_Mode12Hours)
		{
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("1 : AM    2 : PM");
			do
			{
				Alarm_DayTime = KPD_u8GetPressed();
				//when choose invalid input
				if (Alarm_DayTime != NOTPRESSED && (Alarm_DayTime != '1' && Alarm_DayTime != '2'))
				{
					CLCD_vClearScreen();
					CLCD_vSendString("Wrong Choise");
					_delay_ms(500);
					CLCD_vClearScreen();
					CLCD_vSendString("Set Aalrm :");
					CLCD_vSetPosition(2, 6);
					CLCD_vSendData((Alarm_Hour / 10) + 48);
					CLCD_vSendData((Alarm_Hour % 10) + 48);
					CLCD_vSendString(" : ");
					CLCD_vSendData((Alarm_Minutes / 10) + 48);
					CLCD_vSendData((Alarm_Minutes % 10) + 48);
					switch (Alarm_DayTime)
					{
					case 1:
						CLCD_vSendString(" AM");
						break;
					case 0:
						CLCD_vSendString(" PM");
						break;
					}
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("1 : AM    2 : PM");
				}
			} while (Alarm_DayTime != '1' && Alarm_DayTime != '2');

			/*___________________________________________________________________________________________________________________*/

			//Set invaliv val to compre with READ AM or PM Led
			if (Alarm_DayTime == '1')
			{
				Alarm_DayTime = 1; //AM
			}
			else if (Alarm_DayTime == '2')
			{
				Alarm_DayTime = 0; //PM
			}
			else
			{

			}

			/*___________________________________________________________________________________________________________________*/

		}
		//if time mode is 24 hours don't disply AM or PM
		//and set it depend on hour is AM or PM
		else if (Time_Mode == Time_Mode24Hours)
		{
			if (Alarm_Hour >= 12)
			{
				Alarm_DayTime = 0;
			}
			else if (Alarm_Hour >= 0)
			{
				Alarm_DayTime = 1;
			}
			else
			{

			}
		}
		else
		{

		}

		/*___________________________________________________________________________________________________________________*/

		/* Make Alarm Status : Alarm Enable */
		Alarm_Status = Alarm_EN;

		/*___________________________________________________________________________________________________________________*/
	}
	/* if user choose alarm is disaple */
	else if (KPD_Press == '2')
	{
		/* Make Alarm Status : Alarm Disaple */
		Alarm_Hour = NOTPRESSED;
		Alarm_Status = Alarm_DIS;

		/*___________________________________________________________________________________________________________________*/
	}
	else
	{

	}

	/*___________________________________________________________________________________________________________________*/

	/* Check Alarm is correct or not */
	// when it not correct you can ReEnter it by choose Cancel but this option is valid when alarm is enable
	if (Alarm_Status == Alarm_EN)
	{
		CLCD_vClearScreen();
		CLCD_vSendString("Set Alarm ?");
		CLCD_vSetPosition(2, 1);
		CLCD_vSendString("1 : OK    2 : Cancel");
		CLCD_vSetPosition(3, 1);
		CLCD_vSendString("Cancel to Set");
		CLCD_vSetPosition(4, 1);
		CLCD_vSendString("Alarm Again");
		do
		{
			KPD_Press = KPD_u8GetPressed();
			//when choose invalid input
			if (KPD_Press != NOTPRESSED && (KPD_Press != '1' && KPD_Press != '2'))
			{
				CLCD_vClearScreen();
				CLCD_vSendString("Wrong Choise");
				_delay_ms(500);
				CLCD_vClearScreen();
				CLCD_vSendString("Set Alarm ?");
				CLCD_vSetPosition(2, 1);
				CLCD_vSendString("1 : OK    2 : Cancel");
				CLCD_vSetPosition(3, 1);
				CLCD_vSendString("Cancel to Set");
				CLCD_vSetPosition(4, 1);
				CLCD_vSendString("Alarm Again");
			}
		}while (KPD_Press != '1' && KPD_Press != '2');

		/*___________________________________________________________________________________________________________________*/

		//When Alarm is Correct
		if (KPD_Press == '1')
		{
			CLCD_vSendCommand(CLCD_DISPLAYON_CURSOROFF);
			CLCD_vClearScreen();
			CLCD_vSendString("1 - set time");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("2 - select mode");
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("3 - Set Alarm");
			CLCD_vSetPosition(4, 1);
			CLCD_vSendString("4 - Weather");
			CLCD_vSetPosition(4, 15);
			CLCD_vSendString("X:EXIT");
		}
		//When Alarm is not Correct
		else if (KPD_Press == '2')
		{
			Set_Alarm();
		}
		else
		{

		}
		/*___________________________________________________________________________________________________________________*/
	}
	//When Alarm is disaple you don't need to make sure the alarm set correct or not
	else if (Alarm_Status == Alarm_DIS)
	{
		CLCD_vSendCommand(CLCD_DISPLAYON_CURSOROFF);
		CLCD_vClearScreen();
		CLCD_vSendString("1 - set time");
		CLCD_vSetPosition(2, 1);
		CLCD_vSendString("2 - select mode");
		CLCD_vSetPosition(3, 1);
		CLCD_vSendString("3 - Set Alarm");
		CLCD_vSetPosition(4, 1);
		CLCD_vSendString("4 - Weather");
		CLCD_vSetPosition(4, 15);
		CLCD_vSendString("X:EXIT");
	}
	else
	{

	}
	/*___________________________________________________________________________________________________________________*/
}

//======================================================================================================================================//

/* Function To get Temp from LM35 Sensor and display it On CLCD */
void Get_Temp()
{
	volatile u8 LM35_Temp;
	volatile u16 LM35_Volt;
	CLCD_vClearScreen();
	CLCD_vSendString("Press any key");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("To Exit");
	CLCD_vSetPosition(4, 10);
	CLCD_vSendString("TEMP : ");
	while (1)
	{
		//If Any key pressed will exit from Get_Temp Fuction
		KPD_Press = KPD_u8GetPressed();
		if (KPD_Press != NOTPRESSED)
		{
			CLCD_vClearScreen();
			CLCD_vSendString("1 - set time");
			CLCD_vSetPosition(2, 1);
			CLCD_vSendString("2 - select mode");
			CLCD_vSetPosition(3, 1);
			CLCD_vSendString("3 - Set Alarm");
			CLCD_vSetPosition(4, 1);
			CLCD_vSendString("4 - Weather");
			CLCD_vSetPosition(4, 15);
			CLCD_vSendString("X:EXIT");
			break;
		}

		/*___________________________________________________________________________________________________________________*/

		//Get value of Temp Meter
		LM35_u8GetAnalogSignal(&LM350, &LM35_Volt);

		/* Check LM35_Volt */
		/* if it more than 1000 that mean LM35_Temp is positive */
		if (LM35_Volt >= 1000)
		{
			LM35_Temp = (LM35_Volt - 1000) / 10;
			//Display Temp on LCD when it positive
			if (LM35_Temp < 10)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData(LM35_Temp + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
				CLCD_vSendData(' ');
			}
			else if (LM35_Temp < 100)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData((LM35_Temp / 10) + 48);
				CLCD_vSendData((LM35_Temp % 10) + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
			}
			else if (LM35_Temp < 1000)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData((LM35_Temp / 100) + 48);
				CLCD_vSendData(((LM35_Temp / 10) % 10) + 48);
				CLCD_vSendData((LM35_Temp % 10) + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
			}
			else
			{

			}
		}

		/*___________________________________________________________________________________________________________________*/

		/* if it less than 1000 that mean LM35_Temp is negative */
		else
		{
			LM35_Temp = (1000 - LM35_Volt) / 10;
			//Display Temp on LCD when it negative
			if (LM35_Temp < 10)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData('-');
				CLCD_vSendData(LM35_Temp + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
				CLCD_vSendData(' ');
			}
			else if (LM35_Temp < 100)
			{
				CLCD_vSetPosition(4, 16);
				CLCD_vSendData('-');
				CLCD_vSendData((LM35_Temp / 10) + 48);
				CLCD_vSendData((LM35_Temp % 10) + 48);
				CLCD_vSendData(0xDF);
				CLCD_vSendData('C');
			}
			else
			{

			}
		}

		/*___________________________________________________________________________________________________________________*/

	}
}

//======================================================================================================================================//

/* ISR for timer OverFlow
 * each every second Time_Second++
 * check Time_Minute and Time_Hour and Time_Mode
 * ISR will execute with out affects on the code
 */
void ISR_TIMER2_OVF_MODE()
{
	/* it's pluse to make sure that ISR will execute each Second */
	DIO_enumTogglePinVal(DIO_PORTA, DIO_PIN2);

	/*___________________________________________________________________________________________________________________*/

	Time_Second++;

	if (Time_Second == 60)
	{
		Time_Second = 0;
		Time_Minute++;
	}
	if (Time_Minute == 60)
	{
		Time_Minute = 0;
		Time_Hour++;
	}

	/*___________________________________________________________________________________________________________________*/

	/* Check Time condition */
	if (Time_Mode == Time_Mode12Hours)
	{
		if (Time_Hour == 13)
		{
			Time_Hour = 1;
		}
		if (Time_Hour == 12 && Time_Minute == 0 && Time_Second == 0)
		{
			TOG_BIT(PORTA_REG, DIO_PIN3);   //Toggle LED that displays AM or PM
		}
	}

	else if (Time_Mode == Time_Mode24Hours)
	{
		if (Time_Hour == 24)
		{
			Time_Hour = 0;
		}
		else
		{

		}
		if (Time_Hour < 12)
		{
			SET_BIT(PORTA_REG, DIO_PIN3);   //Turn ON Led To Display AM
		}
		else if (Time_Hour == 12 && Time_Minute == 0 && Time_Second == 0)
		{
			CLR_BIT(PORTA_REG, DIO_PIN3);   //Turn OFF Led To Display PM
		}
		else if (Time_Hour > 12)
		{
			CLR_BIT(PORTA_REG, DIO_PIN3);   //Turn OFF Led To Display PM
		}
		else
		{

		}
	}

	else
	{

	}

	/*___________________________________________________________________________________________________________________*/

}
