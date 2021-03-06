// sezione motec ===============================================================

#include <mcp_can.h>
#include <SPI.h>

#define CAN0_INT 2                              // Set INT to pin 2
MCP_CAN CAN0(53);                               // Set CS to pin 53

struct datiMotec {
  uint16_t rpm, map, air, lambda, tps, engtemp, vbat, oilp, oilt, gear, fuel,
  speed, bse, tps2, tpd1, tpd2;
} dm;
long unsigned int rxId;
unsigned char len;
unsigned char rxBuf[8];

void initMotec(){
  // Initialize MCP2515 running at 8Mhz with a baudrate of 1000kb/s and the masks and filters disabled.
  while (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) != CAN_OK){
    //finchè non parte il modulo CAN non si fa niente
    Serial.println("MCP2515 not found");
    delay(1000);
  }
  CAN0.setMode(MCP_NORMAL);// Set operation mode to normal so the MCP2515 sends acks to received data.

  pinMode(CAN0_INT, INPUT);// Configuring pin for /INT input
}
//da 1 se è stato ricevuto il pacchetto di tipo 5, altrimenti 0
int getFromMotec() {
  if (!digitalRead(CAN0_INT)) // If CAN0_INT pin is low, read receive buffer
  {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    if (rxId != 0 && len == 8) { //ogni pacchetto CAN deve avere 8 byte di dati, cioè 4 uint16

      //spacchetto i dati per il mio struct e li metto in scala se necessario
      if (rxId == 2) {
        dm.rpm = ((uint16_t)rxBuf[0] << 8) | rxBuf[1] ;
        dm.rpm /= 100;
        dm.map = ((uint16_t)rxBuf[2] << 8) | rxBuf[3] ;
        dm.air = ((uint16_t)rxBuf[4] << 8) | rxBuf[5] ;
        dm.lambda = ((uint16_t)rxBuf[6] << 8) | rxBuf[7] ;
      } else if (rxId == 3) {
        dm.tps = ((uint16_t)rxBuf[0] << 8) | rxBuf[1] ;
        dm.engtemp = ((uint16_t)rxBuf[2] << 8) | rxBuf[3] ;
        dm.engtemp /= 10;
        dm.vbat = ((uint16_t)rxBuf[4] << 8) | rxBuf[5] ;
        dm.vbat /= 10;
        dm.oilp = ((uint16_t)rxBuf[6] << 8) | rxBuf[7] ;
        dm.oilp /= 100;
      } else if (rxId == 4) {
        dm.oilt = ((uint16_t)rxBuf[0] << 8) | rxBuf[1] ;
        dm.gear = ((uint16_t)rxBuf[2] << 8) | rxBuf[3] ;
        dm.fuel = ((uint16_t)rxBuf[4] << 8) | rxBuf[5] ;
        dm.speed = ((uint16_t)rxBuf[6] << 8) | rxBuf[7] ;
        dm.speed /= 10;
      } else if (rxId == 5) {
        dm.bse = ((uint16_t)rxBuf[0] << 8) | rxBuf[1] ;
        dm.tps2 = ((uint16_t)rxBuf[2] << 8) | rxBuf[3] ;
        dm.tpd1 = ((uint16_t)rxBuf[4] << 8) | rxBuf[5] ;
        dm.tpd2 = ((uint16_t)rxBuf[6] << 8) | rxBuf[7] ;
        return 1;
      }
    }
  }
  return 0;

}

//sezione cruscotto ============================================================

struct datiCrusc {
  uint8_t rpm, gear, speed, engtemp, oilp, vbat, lambda, oilt;
} dc;

void updateDashboard() {
  //preparo il pacchetto per il cruscotto dati troncati a 8 bit
  dc.rpm      = dm.rpm;
  dc.gear     = dm.gear;
  dc.speed    = dm.speed;
  dc.engtemp  = dm.engtemp;
  dc.oilp     = dm.oilp;
  dc.vbat     = dm.vbat;

  dc.lambda = dm.lambda >> 5;
  // --- il valore della motec andrebbe diviso per circa 283 ---
  // io qui shifto di 5 bit che equivale a dividere per 32 in modo da avere le
  // cifre significative in un unico byte. infatti i valori del sensore vanno da
  // 0 a poco più di 5000, dividendo per 32 ottengo valori entro il range di
  // un solo byte (0-255), le operazioni in virgola mobile le lascio fare al
  // cruscotto che deve ulteriormente dividere per 8.84375 per ottenere il
  // valore finale da stampare sul display
  dc.oilt = dm.oilt /10;
  //Cruscotto accetta pacchetti da 6 preceduti dal byte 204
  Serial3.write(204);
  Serial3.write((char*)&dc, sizeof(dc)); //vs cruscotto
  //Serial3.write((char*)&dc, sizeof(dc)-1);//se non vuoi la lambda on the dash
  
}

//sezione imu ==================================================================

#include <Wire.h>
#include "i2c.h"

// IMU-Sensor
#include "i2c_MPU9250.h"
MPU9250 mpu9250;

void setupIMU(){
  Serial.print("Probe MPU9250: ");
    switch (mpu9250.initialize())
    {
        case 0: Serial.println("MPU-Sensor missing"); while(1) {};
        case 1: Serial.println("Found unknown Sensor."); break;
        case 2: Serial.println("MPU6500 found."); break;
        case 3: Serial.println("MPU9250 found!"); break;
    }

    Serial.print("Probe AK8963: ");
    if (i2c.probe(0x0C)) Serial.println("AK8963 found!");
    else                 Serial.println("AK8963 missing");

}

//sezione daq ==================================================================
struct datiDinamici { //4+2*9+2*6 = 34 byte
  uint32_t t;
  uint16_t a5,a6,a7,a9,a10,a11,a12,a13,a14;
  int16_t ax,ay,az,gx,gy,gz;
} dd;

void daq(){
  dd.a5   = analogRead(A5);
  dd.a6   = analogRead(A6); //ntc
  dd.a7   = analogRead(A7);

  dd.a9   = analogRead(A0); //rear brake
  dd.a10  = analogRead(A1);
  dd.a11  = analogRead(A2);
  dd.a12  = analogRead(A3);
  dd.a13  = analogRead(A4);
  dd.a14  = analogRead(A5); //steer not connected

  dd.t    = millis();
  
  static float fvec[9];
  mpu9250.getMeasurement(fvec);
  
  dd.ax   = fvec[0]*1000;
  dd.ay   = fvec[1]*1000;
  dd.az   = fvec[2]*1000;
  dd.gx   = fvec[4]*10;
  dd.gy   = fvec[5]*10;
  dd.gz   = fvec[6]*10;
  
}

//==============================================================================

void setup()
{
  Serial.begin(115200); //vs usb
  Serial2.begin(115200); //vs raspi
  Serial3.begin(4800); //vs cruscotto
  setupIMU();
  initMotec();

}

int Tcrusc = 100; //periodo in millisecondi tra i frame mandati al cruscotto
int lastcrusc = 0;

int Tdaq = 10; //periodo in millisecondi tra acquisizioni
int lastDaq = 0;
int time;
void loop()
{
  getFromMotec();
  time = millis();
  if (time - lastDaq >= Tdaq) {
    lastDaq = time;
    daq();
    //invio a raspberry pi (occhio alle tensioni!!!! 3.3V vs 5V!!)
    Serial2.write(0x7b); //arduinoheaderlow
    Serial2.write(0xea); //arduinoheaderHigh
    Serial2.write((char*)&dm, sizeof(dm));
    Serial2.write((char*)&dd, sizeof(dd));
  }


  //se il cruscotto da problemi provare con un ignorantissimo delay(100)
  if (time - lastcrusc >= Tcrusc) {
    lastcrusc = time;
    updateDashboard();
  }

}
