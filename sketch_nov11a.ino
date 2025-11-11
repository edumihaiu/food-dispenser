#include <avr/io.h>
#include <avr/interrupt.h>

#define DT 2
#define SCLK 3

typedef struct {
	int32_t offset;
	float scale;
} HX711;

int my_digitalRead(uint8_t pin) {
	volatile uint8_t *pinReg;
	uint8_t bitmask;
	
	if (pin <=7) {
		pinReg = &PIND;
		bitmask = (1 << pin);
	}
	else if (pin <=13) {
		pinReg = &PINB;
		bitmask = (1 << (pin - 8));
	} else if (pin <=18) {
		pinReg = &PINC;
		bitmask = (1 << (pin - 14));
	} else return 0;

	return (*pinReg & bitmask) ? 1 : 0;
}

int32_t readHX711_raw(uint8_t DOUT_PIN, uint8_t SCLK_PIN) {
	while(my_digitalRead(DOUT_PIN)); // wait until DOUT is LOW

	int32_t value = 0;
	uint8_t i;
	cli(); // no interrupts
	for (i = 0; i < 24; ++i) {
		PORTD |= (1 << SCLK_PIN); // write HIGH SCLK

		value = value << 1;
		value |= my_digitalRead(DOUT_PIN);

		PORTD &= ~(1 << SCLK_PIN); // write LOW SCLK
	}

	if (value & 0x800000L) {
		// check if MSB is HIGH; if it is -> negative
		// if it is fill from 31 to 24 with HIGH
		value |= 0xFF000000L;
	}

	PORTD |= (1 << SCLK_PIN); // set channel A 128 rate
	PORTD &= ~(1 << SCLK_PIN);
	sei(); // interrupts back
	return value;
}

int32_t read_avgHX711(uint8_t n) {
	int64_t sum = 0;
	uint8_t i;
	for (i = 0; i < n; ++i) {
		sum += readHX711_raw(DT, SCLK);
	}
	return (int32_t)(sum / n);
}

void HX711_tare(HX711* value) {
	value->offset = read_avgHX711(10);
}

void HX711_set_scale(HX711* value, float scale) {
	value->scale = scale;
}

float HX711_get_weight(HX711* value) {
	return (read_avgHX711(10) - value->offset) / value->scale;
}

void setup() {
  // put your setup code here, to run once:
  DDRD &= ~(1 << DDD2); // DT intrare
	DDRD |= (1 << DDD3); // SCLK iesire
	
	PORTD |= (1 << DDD2); // pullup DT
	PORTD &= ~(1 << DDD3); // low SCLK	
	Serial.begin(9600);

}

void loop() {
	HX711 cantar;
	cantar.scale = 1;
	cantar.offset = 0;
	HX711_set_scale(&cantar, -1000.445);
	Serial.println("Tare: wait a few seconds");
	delay(5000);
	HX711_tare(&cantar);
	Serial.println("Place weight");
	delay(5000);
	Serial.println(HX711_get_weight(&cantar));

}
