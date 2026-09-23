#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <dht.h>

dht DHT;
#define DHT22_PIN   2

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS    2

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

void setup() {
	Serial.begin(9600);

	lcd.init();
	lcd.backlight();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Env Monitor Init");
	delay(1500);
	lcd.clear();
}

void loop() {
	int result = DHT.read22(DHT22_PIN);

	Serial.print("Read result: ");
	Serial.println(result);  

	if (result != 0) {
		Serial.println("Failed to read from DHT sensor!");
		lcd.setCursor(0, 0);
		lcd.print("Sensor Error    ");
		lcd.setCursor(0, 1);
		lcd.print("Check wiring    ");
		delay(2000);
		return;
	}

	float humidity    = DHT.humidity;
	float temperature = DHT.temperature;

	Serial.print("Temperature: ");
	Serial.print(temperature);
	Serial.print(" C   Humidity: ");
	Serial.print(humidity);
	Serial.println(" %");

	lcd.setCursor(0, 0);
	lcd.print("Temp: ");
	lcd.print(temperature);
	lcd.print((char)223);
	lcd.print("C   ");

	lcd.setCursor(0, 1);
	lcd.print("Hum:  ");
	lcd.print(humidity);
	lcd.print(" %   ");

	delay(2000);
}