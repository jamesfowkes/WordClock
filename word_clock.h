#ifndef _WORD_CLOCK_H_
#define _WORD_CLOCK_H_

/*
 * Defines and Typedefs
 */

#define BUTTON_SCAN_PERIOD_MS (10)

bool WC_BTN_Init(uint8_t scanPeriodMs);
void WC_BTN_Tick(BTN_STATE_ENUM hr, BTN_STATE_ENUM min);

void WC_IncrementHour(void);
void WC_IncrementMinute(void);

#endif
