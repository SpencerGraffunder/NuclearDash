#ifndef CONFIG_H
#define CONFIG_H

// Common


#ifdef ESP32S3 // ESP32S3 specific
	#define CAN_TX_PIN GPIO_NUM_2
	#define CAN_RX_PIN GPIO_NUM_3
	#define PIN_BEEP 9
#else // ESP32 specific
	#define CAN_TX_PIN GPIO_NUM_33
	#define CAN_RX_PIN GPIO_NUM_13
	#define PIN_BEEP 17
#endif

#endif // CONFIG_H