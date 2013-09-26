/*
 * Standard Library Includes
 */

#include <stdbool.h>
#include <stdint.h>

/*
 * Generic Library Includes
 */

#include "button.h"

/*
 * Local Application Includes
 */

#include "word_clock.h"

/*
 * Defines and Typedefs
 */

#define BUTTON_DEBOUNCE_MS (100)
#define BUTTON_REPEAT_MS (1000)
#define BUTTON_REPEAT_COUNT (BUTTON_REPEAT_MS / BUTTON_SCAN_PERIOD_MS)
#define BUTTON_DEBOUNCE_COUNT (BUTTON_DEBOUNCE_MS / BUTTON_SCAN_PERIOD_MS)

#define IDLE_MS_COUNT (2000)

/*
 * Private Function Prototypes
 */

void hrBtnRepeat(void);
void hrBtnChange(BTN_STATE_ENUM state);

void minBtnRepeat(void);
void minBtnChange(BTN_STATE_ENUM state);

/*
 * Local Variables
 */
 
static BTN hrButton = 
{
	.current_state = BTN_STATE_INACTIVE,
	.change_state_callback = hrBtnChange,
	.repeat_callback = hrBtnRepeat,
	.max_repeat_count = BUTTON_REPEAT_COUNT,
	.max_debounce_count = BUTTON_DEBOUNCE_COUNT
};

static BTN minButton = 
{
	.current_state = BTN_STATE_INACTIVE,
	.change_state_callback = minBtnChange,
	.repeat_callback = minBtnRepeat,
	.max_repeat_count = BUTTON_REPEAT_COUNT,
	.max_debounce_count = BUTTON_DEBOUNCE_COUNT
};

static uint8_t s_scanPeriodMs;

/*
 * Public Functions
 */
 
bool WC_BTN_Init(uint8_t scanPeriodMs)
{
	bool success = true;
	
	s_scanPeriodMs = scanPeriodMs;
	
	success &= BTN_InitHandler(&hrButton);
	success &= BTN_InitHandler(&minButton);
	
	return success;
}
 
void WC_BTN_Tick(BTN_STATE_ENUM hr, BTN_STATE_ENUM min)
{
	BTN_Update(&hrButton, hr);
	BTN_Update(&minButton, min);
}

/*
 * Private Functions
 */

void hrBtnRepeat(void)
{
	WC_IncrementHour();
}

void hrBtnChange(BTN_STATE_ENUM state)
{
	if (state == BTN_STATE_ACTIVE)
	{
		hrBtnRepeat();
	}
}

void minBtnRepeat(void)
{
	WC_IncrementMinute();
}

void minBtnChange(BTN_STATE_ENUM state)
{
	if (state == BTN_STATE_ACTIVE)
	{
		minBtnRepeat();
	}
}
