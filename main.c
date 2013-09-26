/*
 * Standard Library Includes
 */

#include <stdbool.h>
#include <stdint.h>

/*
 * Utility Includes
 */

#include "util_macros.h"
#include "util_time.h"

/*
 * Generic Library Includes
 */

#include "button.h"

/*
 * AVR Includes (Defines and Primitives)
 */
#include <avr/io.h>
#include <avr/wdt.h>
/*
 * AVR Library Includes
 */

#include "lib_i2c_common.h"
#include "lib_clk.h"
#include "lib_tmr8_tick.h"

/*
 * Device Includes
 */

#include "lib_ds3231.h"

/*
 * Local Application Includes
 */

#include "word_clock.h"

/*
 * Private Defines and Datatypes
 */

#define APPLICATION_MS_TICK (1000)
#define BUTTON_DELAY_MS (2000)

#define HR_PORT PORTB
#define HR_PIN 0
#define MIN_PORT PORTB
#define MIN_PIN 1

/*
 * The word clock uses chained TLC5916 registers
 * Define each word as bit location
 */
// Register 0
#define	FIVE_MINS	(0)
#define	TEN_MINS	(1)
#define	QUARTER		(2)
#define	TWENTY_MINS	(3)
#define	HALF		(4)
#define	PAST		(5)
#define	TO			(6)
#define	ONE			(7)
// Register 1
#define	TWO			(8)
#define	THREE		(9)
#define	FOUR		(10)
#define	FIVE		(11)
#define	SIX			(12)
#define	SEVEN		(13)
#define	EIGHT		(14)
#define	NINE		(15)
// Register 2
#define	TEN			(16)
#define	ELEVEN		(17)
#define	TWELVE		(18)
#define	MORNING		(19)
#define	AFTERNOON	(20)
#define	EVENING		(21)

// Since each register is 8-bits wide, divide by 8 to get register for that bit
#define REGISTER(x) (x / 8)
// The location of each bit within its 8-bit wide register
#define BIT_LOC(x) (x - (REGISTER(x) * 8))
// The bitmask for that bit
#define BIT(x) (1 << (x - (REGISTER(x) * 8)))

enum states
{
	DISPLAY,
	EDITING,
	WRITING,
};
typedef enum states STATES;

/*
 * Private Function Prototypes
 */
 
static void setupTimer(void);
static void updateClock(void);

static void updateMinutes(uint8_t tlcData[], uint16_t secondsThroughHour);
static void updateHour(uint8_t tlcData[], uint8_t hours);

static void onTimeUpdateComplete(bool write);

/*
 * Private Variables
 */
 
/* Library control structures */
static TMR8_TICK_CONFIG appTick;
static TMR8_TICK_CONFIG btnTick;

static TMR8_DELAY_CONFIG buttonDelay;

static STATES state = DISPLAY;

static TM time;

int main(void)
{
	/* Disable watchdog: not required for this application */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	
	setupTimer();
	
	DS3231_Init();
	
	WC_BTN_Init(BUTTON_SCAN_PERIOD_MS);
	
	while (true)
	{
		if (TMR8_Tick_TestAndClear(&appTick))
		{
			if (state == DISPLAY)
			{
				DS3231_ReadDateTime(onTimeUpdateComplete);
			}
		}
		
		if (TMR8_Tick_TestAndClear(&btnTick))
		{
			BTN_STATE_ENUM hr = IO_Read(HR_PORT, HR_PIN);
			BTN_STATE_ENUM min = IO_Read(MIN_PORT, MIN_PIN);
			
			WC_BTN_Tick(hr, min);
		}
		
		if (TMR8_Tick_TestDelayAndClear(&buttonDelay))
		{
			state = WRITING;
			DS3231_SetDateTime(&time, false, onTimeUpdateComplete);
		}
	}
}

static void onTimeUpdateComplete(bool write)
{
	(void)write;
	state = DISPLAY;
	DS3231_GetTime(&time);
	updateClock();
}

static void updateClock(void)
{
	uint8_t tlcData[3] = {0, 0, 0};
	
	uint16_t secondsThroughHour = (time.tm_min * 60) + time.tm_sec;
	updateMinutes(tlcData, secondsThroughHour);
	
	if (secondsThroughHour >= MINS_SECS_TO_SECS(37, 30))
	{
		// Need to show "to" the hour
		updateHour(tlcData, (uint8_t)time.tm_hour + 1);
		tlcData[REGISTER(TO)] |= BIT(TO);
	}
	else
	{
		// Need to show "past" the hour
		updateHour(tlcData, (uint8_t)time.tm_hour);
		tlcData[REGISTER(PAST)] |= BIT(PAST);
	}
	
	if (time.tm_hour < 12)
	{
		tlcData[REGISTER(MORNING)] |= BIT(MORNING);
	}
	else if (time.tm_hour < 15)
	{
		tlcData[REGISTER(AFTERNOON)] |= BIT(AFTERNOON);
	}
	else
	{
		tlcData[REGISTER(EVENING)] |= BIT(EVENING);
	}
}

static void updateMinutes(uint8_t tlcData[], uint16_t secondsThroughHour)
{
	if ((secondsThroughHour < MINS_SECS_TO_SECS(2, 30)) ||
		(secondsThroughHour > MINS_SECS_TO_SECS(57, 30)))
	{
		// Do nothing : no minutes to display
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(7, 30))
	{
		// Treat as five past
		tlcData[REGISTER(FIVE_MINS)] |= BIT(FIVE_MINS);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(12, 30))
	{
		// Treat as ten past
		tlcData[REGISTER(TEN_MINS)] |= BIT(TEN_MINS);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(17, 30))
	{
		// Treat as quarter to
		tlcData[REGISTER(QUARTER)] |= BIT(QUARTER);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(22, 30))
	{
		// Treat as twenty past
		tlcData[REGISTER(TWENTY_MINS)] |= BIT(TWENTY_MINS);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(27, 30))
	{
		// Treat as twenty-five past
		tlcData[REGISTER(FIVE_MINS)] |= BIT(FIVE_MINS);
		tlcData[REGISTER(TWENTY_MINS)] |= BIT(TWENTY_MINS);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(32, 30))
	{
		// Treat as half past
		tlcData[REGISTER(HALF)] |= BIT(HALF);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(37, 30))
	{
		// Treat as twenty-five to
		tlcData[REGISTER(FIVE_MINS)] |= BIT(FIVE_MINS);
		tlcData[REGISTER(TWENTY_MINS)] |= BIT(TWENTY_MINS);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(42, 30))
	{
		// Treat as twenty to
		tlcData[REGISTER(TWENTY_MINS)] |= BIT(TWENTY_MINS);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(47, 30))
	{
		// Treat as quarter to
		tlcData[REGISTER(QUARTER)] |= BIT(QUARTER);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(52, 30))
	{
		// Treat as ten to
		tlcData[REGISTER(TEN_MINS)] |= BIT(TEN_MINS);
	}
	else if (secondsThroughHour < MINS_SECS_TO_SECS(57, 30))
	{
		// Treat as five to
		tlcData[REGISTER(FIVE_MINS)] |= BIT(FIVE_MINS);
	}
}

static void updateHour(uint8_t tlcData[], uint8_t hour)
{
	if ((hour == 0) || (hour == 12)) { tlcData[REGISTER(TWELVE)] |= BIT(TWELVE); }
	else if ((hour == 1) || (hour == 13)) { tlcData[REGISTER(ONE)] |= BIT(ONE); }
	else if ((hour == 2) || (hour == 14)) { tlcData[REGISTER(TWO)] |= BIT(TWO); }
	else if ((hour == 3) || (hour == 15)) { tlcData[REGISTER(THREE)] |= BIT(THREE); }
	else if ((hour == 4) || (hour == 16)) { tlcData[REGISTER(FOUR)] |= BIT(FOUR); }
	else if ((hour == 5) || (hour == 17)) { tlcData[REGISTER(FIVE)] |= BIT(FIVE); }
	else if ((hour == 6) || (hour == 18)) { tlcData[REGISTER(SIX)] |= BIT(SIX); }
	else if ((hour == 7) || (hour == 19)) { tlcData[REGISTER(SEVEN)] |= BIT(SEVEN); }
	else if ((hour == 8) || (hour == 20)) { tlcData[REGISTER(EIGHT)] |= BIT(EIGHT); }
	else if ((hour == 9) || (hour == 21)) { tlcData[REGISTER(NINE)] |= BIT(NINE); }
	else if ((hour == 10) || (hour == 22)) { tlcData[REGISTER(TEN)] |= BIT(TEN); }
	else if ((hour == 11) || (hour == 23)) { tlcData[REGISTER(ELEVEN)] |= BIT(ELEVEN); }
}

static void setupTimer(void)
{
	CLK_Init(0);
	TMR8_Tick_Init();

	appTick.reload = APPLICATION_MS_TICK;
	appTick.active = true;
	TMR8_Tick_AddTimerConfig(&appTick);
	
	btnTick.reload = BUTTON_SCAN_PERIOD_MS;
	btnTick.active = true;
	TMR8_Tick_AddTimerConfig(&btnTick);
}

void WC_IncrementHour(void)
{
	state = EDITING;
	uint32_t seconds = time_to_unix_seconds(&time);
	seconds += S_PER_HOUR;
	unix_seconds_to_time(seconds, &time);
}

void WC_IncrementMinute(void)
{
	state = EDITING;
	uint32_t seconds = time_to_unix_seconds(&time);
	seconds += (5 * S_PER_MIN);
	unix_seconds_to_time(seconds, &time);
}

void WC_ButtonsIdle(void)
{
	buttonDelay.delayMs = BUTTON_DELAY_MS;
	TMR8_Tick_StartDelay(&buttonDelay);
}