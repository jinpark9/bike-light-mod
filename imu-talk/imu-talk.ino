#include <Wire.h>
#include <SPI.h>

#define WHO_AM_I_MPU9250    0x75
#define MPU9250_ADDRESS     0x68
#define INT_STATUS          0x3A
#define ACCEL_XOUT_H        0x3B
#define GYRO_XOUT_H         0x43

#define AK8963_ADDRESS      0x0C
#define AK8963_WHO_AM_I     0x00 // should return 0x48
#define AK8963_ST1          0x02  // data ready status bit 0
#define AK8963_XOUT_L       0x03

#define INT_PIN_CFG      0x37
#define INT_ENABLE       0x38

//timer defines
#define NUM_TIMERS 10

#define LED 5
#define LED_OFF_TIME 100

unsigned long timers[NUM_TIMERS][3]; // [timer][last time, elapse request, active status]

int16_t gyro[3];
int16_t accel[3];
int16_t mag[3];
uint16_t led[4]; // [active_status, on/off state, rate, timer#]

uint16_t blink_duration = 1000;

void setup() {
  // put your setup code here, to run once:
  setupAllTimers();
  
  pinMode(LED, OUTPUT); // initialize LED pin
  digitalWrite(LED, HIGH);
  led[0] = 0; // start LED as off
  
  Serial.begin(115200);
  Wire.begin();  // I2C haberlesme baslatir

  writeByte(MPU9250_ADDRESS, INT_PIN_CFG, 0x22);    
  writeByte(MPU9250_ADDRESS, INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
  delay(100);

  byte c = readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);  // Read WHO_AM_I register for MPU-9250
  Serial.print("MPU9250 ");
  Serial.print("I AM ");
  Serial.print(c, HEX);
  Serial.print(" I should be ");
  Serial.println(0x71, HEX);

  byte d = readByte(AK8963_ADDRESS,AK8963_WHO_AM_I);
  Serial.print("AK8963 ");
  Serial.print("I AM ");
  Serial.print(d, HEX);
  Serial.print(" I should be ");
  Serial.println(0x48, HEX);
  startTimer(0,5000);
}

void loop() {  
//  if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01) {
//    uint8_t rawData[6];
//    readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, &rawData[0]);
//    int16_t gx = ((int16_t)rawData[0] << 8) | rawData[1];
//    int16_t gy = ((int16_t)rawData[2] << 8) | rawData[3];
//    int16_t gz = ((int16_t)rawData[4] << 8) | rawData[5] ;
//  }


//TEST blink rate switching
  if (isTimeout(0)) {
    if (blink_duration == 100) {
      blink_duration = 1000;
    }
    else {
      blink_duration = 100;
    }
    startTimer(0,5000);
  }
  blink(blink_duration);
  
}

void blink(uint16_t duration) {
  led[2] = duration;
  
  if (led[0] == 0) { // activate light, turn on, assign duration, and assign timer
    led[0] = 1;
    digitalWrite(LED, LOW);
    led[1] = 1;
    led[3] = getAvailableTimer();
    startTimer(led[3], led[2]);
  }

  // if there's a timeout, check the on/off state and toggle
  if (isTimeout(led[3])) {
    if (led[1] == 1) {
      // turn off LED, start timer
      digitalWrite(LED, HIGH);
      led[1] = 0;
      startTimer(led[3], LED_OFF_TIME);
    }
    else {
      // turn on LED, start timer
      digitalWrite(LED, LOW);
      led[1] = 1;
      startTimer(led[3], led[2]);
    }
  }
}

void setupAllTimers() {
  for (int i = 0; i < NUM_TIMERS; i++) {
    timers[i][2] = 0; //sets timers to inactive
  }
}

int getAvailableTimer() {
  for (int i = 0; i < NUM_TIMERS; i++) {
    if (timers[i][2] == 0) {
      Serial.print("got a new timer at #");
      Serial.println(i);
      return i;
    }
  }
}

void startTimer(uint8_t timer, unsigned long timeLen) {
  timers[timer][0] = millis();
  timers[timer][1] = timeLen;
  timers[timer][2] = 1;
}

bool isTimeout(uint8_t timer) {
  if (timers[timer][2] != 1) {
    return false;
  }
  return (millis() - timers[timer][0] > timers[timer][1]);
}

void writeByte(uint8_t address, uint8_t subAddress, uint8_t data) {
  Wire.beginTransmission(address);  // Initialize the Tx buffer
  Wire.write(subAddress);           // Put slave register address in Tx buffer
  Wire.write(data);                 // Put data in Tx buffer
  Wire.endTransmission();           // Send the Tx buffer
}

uint8_t readByte(uint8_t address, uint8_t subAddress) {
  uint8_t data; // `data` will store the register data   
  Wire.beginTransmission(address);         // Initialize the Tx buffer
  Wire.write(subAddress);                  // Put slave register address in Tx buffer
  Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
  Wire.requestFrom(address, (uint8_t) 1);  // Read one byte from slave register address 
  data = Wire.read();                      // Fill Rx buffer with result
  return data;                             // Return data read from slave register
}

void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{  
  Wire.beginTransmission(address);   // Initialize the Tx buffer
  Wire.write(subAddress);            // Put slave register address in Tx buffer
  Wire.endTransmission(false);       // Send the Tx buffer, but send a restart to keep connection alive
  uint8_t i = 0;
  Wire.requestFrom(address, count);  // Read bytes from slave register address 
  
  while (Wire.available()) {         // Put read results in the Rx buffer
    dest[i++] = Wire.read();
  }
}

void readMagData(int16_t * destination)
{
  uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition
  if (readByte(AK8963_ADDRESS, AK8963_ST1) & 0x01) { // wait for magnetometer data ready bit to be set
    readBytes(AK8963_ADDRESS, AK8963_XOUT_L, 7, &rawData[0]);  // Read the six raw data and ST2 registers sequentially into data array
    uint8_t c = rawData[6]; // End data read by reading ST2 register
    if (!(c & 0x08)) { // Check if magnetic sensor overflow set, if not then report data
      destination[0] = ((int16_t)rawData[1] << 8) | rawData[0] ;  // Turn the MSB and LSB into a signed 16-bit value
      destination[1] = ((int16_t)rawData[3] << 8) | rawData[2] ;  // Data stored as little Endian
      destination[2] = ((int16_t)rawData[5] << 8) | rawData[4] ; 
    }
  }
}
