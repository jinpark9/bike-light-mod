#include <Wire.h>


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
    Wire.begin();  // I2C haberlesme baslatir

  
//   baseline = getPressure();
  
//  Serial.print("baseline pressure: ");
//  Serial.print(baseline);
//  Serial.println(" mb");  
  delay(500);
  
//    write(0x6B, 0); //Guc yonetimi registeri default:0
//    write(0x6A, 0);  // I2C master kapali, acik olmasini istiyorsaniz 0x20 olmali
//    write(0x37, 0x02); //Bypass modu acik
//    writeMag(0x0A, 0x12); // surekli olcebilmek icin manyetik sensor registeri
}

int reading = 0;

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < 13; i++) {
    Wire.requestFrom(0x0C,1);
    uint8_t n = Wire.read();
    Serial.println(n);
  } 
  Serial.println("---");
}

void write(int reg, int data)
{
    Wire.beginTransmission(0x68); // 0x68 sensor adresine veri transferi baslar
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission(true);
}

void writeMag(int reg, int data)
{
    Wire.beginTransmission(0x0C);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission(true);
}
