#include <Arduino.h>
#include <U8g2lib.h>
#include <bitset>

// Constants
const uint32_t interval = 100; //Display update interval
// The following is based on the frequencies: 261.626, 277.183, 293.665,  311.127,  329.628, 349.228, 369.994, 391.995, 415.305, 440, 466.164, 493.883
const uint32_t stepSizes [] = {51076142, 54113269, 57330981, 60740013, 64351885, 68178311, 72232370, 76527532, 81078245, 85899346, 9007233,  96418697};
const char *note_map[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Global Variables
volatile uint32_t currentStepSize = 0;
static uint32_t phaseAcc = 0;

//Pin definitions
//Row select and enable
const int RA0_PIN = D3;
const int RA1_PIN = D6;
const int RA2_PIN = D12;
const int REN_PIN = A5;

//Matrix input and output
const int C0_PIN = A2;
const int C1_PIN = D9;
const int C2_PIN = A6;
const int C3_PIN = D1;
const int OUT_PIN = D11;

//Audio analogue out
const int OUTL_PIN = A4;
const int OUTR_PIN = A3;

//Joystick analogue in
const int JOYY_PIN = A0;
const int JOYX_PIN = A1;

//Output multiplexer bits
const int DEN_BIT = 3;
const int DRST_BIT = 4;
const int HKOW_BIT = 5;
const int HKOE_BIT = 6;

//Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

//Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value) {
	digitalWrite(REN_PIN,LOW);
	digitalWrite(RA0_PIN, bitIdx & 0x01);
	digitalWrite(RA1_PIN, bitIdx & 0x02);
	digitalWrite(RA2_PIN, bitIdx & 0x04);
	digitalWrite(OUT_PIN,value);
	digitalWrite(REN_PIN,HIGH);
	delayMicroseconds(2);
	digitalWrite(REN_PIN,LOW);
}

std::bitset<4> readCols(){
	std::bitset<4> result;
	result[0] = digitalRead(C0_PIN);
	result[1] = digitalRead(C1_PIN);
	result[2] = digitalRead(C2_PIN);
	result[3] = digitalRead(C3_PIN);
	return result;
}

void setRow(uint8_t rowIdx){
	digitalWrite(REN_PIN, LOW);
	digitalWrite(RA0_PIN, rowIdx & 0x01);
	digitalWrite(RA1_PIN, rowIdx & 0x02);
	digitalWrite(RA2_PIN, rowIdx & 0x04);
	digitalWrite(REN_PIN, HIGH);
}

void sampleISR() {
	// Serial.println("Interrupt");
	// static uint32_t phaseAcc = 0;

	phaseAcc += currentStepSize;
	Serial.println(currentStepSize);
	Serial.println(phaseAcc);
	int32_t Vout = (phaseAcc >> 24) - 128;
	Serial.println(Vout);
	Serial.println();
	analogWrite(OUTR_PIN, (Vout + 128)>>4);
}

void setup() {
	// put your setup code here, to run once:

	//Set pin directions
	pinMode(RA0_PIN, OUTPUT);
	pinMode(RA1_PIN, OUTPUT);
	pinMode(RA2_PIN, OUTPUT);
	pinMode(REN_PIN, OUTPUT);
	pinMode(OUT_PIN, OUTPUT);
	pinMode(OUTL_PIN, OUTPUT);
	pinMode(OUTR_PIN, OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);

	pinMode(C0_PIN, INPUT);
	pinMode(C1_PIN, INPUT);
	pinMode(C2_PIN, INPUT);
	pinMode(C3_PIN, INPUT);
	pinMode(JOYX_PIN, INPUT);
	pinMode(JOYY_PIN, INPUT);

	//Initialise display
	setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
	delayMicroseconds(2);
	setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
	u8g2.begin();
	setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

	//Initialise UART
	Serial.begin(9600);
	Serial.println("Hello World");

	TIM_TypeDef *Instance = TIM1;
	HardwareTimer *sampleTimer = new HardwareTimer(Instance);
	sampleTimer->setOverflow(22000, HERTZ_FORMAT);
	sampleTimer->attachInterrupt(sampleISR);
	sampleTimer->resume();
}

void loop() {
	// put your main code here, to run repeatedly:
	static uint32_t next = millis();

	std::bitset<32> inputs;

	for(uint8_t i = 0; i <= 2; i++){
		setRow(i);
		delayMicroseconds(3);
		std::bitset<4> result = readCols(); 
		inputs[(i * 4)] = result[0];
		inputs[(i * 4) + 1] = result[1];
		inputs[(i * 4) + 2] = result[2];
		inputs[(i * 4) + 3] = result[3];
	}

	bool key_pressed = false;
	for(uint8_t i = 0; i <= 12; i++){
		if(inputs[i] == 0){
			u8g2.drawStr(2,30,note_map[i]);
			currentStepSize = stepSizes[i];
			key_pressed = true;
		}
	}
	if(!key_pressed){
		currentStepSize = 0;
		// phaseAcc = 0;
	}

	while (millis() < next);  //Wait for next interval

	next += interval;

	//Update display
	u8g2.clearBuffer();         // clear the internal memory
	u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
	u8g2.drawStr(2,10,"Helllo World!");  // write something to the internal memory
	u8g2.setCursor(2,20);
	u8g2.print(inputs.to_ulong(),HEX);

	u8g2.sendBuffer();          // transfer internal memory to the display

	//Toggle LED
	digitalToggle(LED_BUILTIN);
  
}