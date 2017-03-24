// sezione motec ===============================================================

#include <mcp_can.h>
#include <SPI.h>

#define CAN0_INT 2                              // Set INT to pin 2
MCP_CAN CAN0(53);                               // Set CS to pin 10

struct datiMotec {
  uint16_t rpm, map, air, lambda, tps, engtemp, vbat, oilp, oilt, gear, fuel,
  speed, bse, tps2, tpd1, tpd2;
} dm;
long unsigned int rxId;
unsigned char len;
unsigned char rxBuf[8];

void initMotec(){
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) != CAN_OK){
    //finchè non parte il modulo CAN non si fa niente
    Serial.println("MCP2515 not found");
    while(1) ;
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
        dm.bse *= 10;
        dm.tps2 = ((uint16_t)rxBuf[2] << 8) | rxBuf[3] ;
        dm.tps2 *= 10;
        dm.tpd1 = ((uint16_t)rxBuf[4] << 8) | rxBuf[5] ;
        dm.tpd1 *= 10;
        dm.tpd2 = ((uint16_t)rxBuf[6] << 8) | rxBuf[7] ;
        dm.tpd2 *= 10;
        return 1;
      }
    }
  }
  return 0;

}

//sezione cruscotto ============================================================

struct datiCrusc {
  uint8_t rpm, gear, speed, engtemp, oilp, vbat;
} dc;

void updateDashboard() {
  //preparo il pacchetto per il cruscotto
  dc.rpm      = dm.rpm;
  dc.gear     = dm.gear;
  dc.speed    = dm.speed;
  dc.engtemp  = dm.engtemp;
  dc.oilp     = dm.oilp;
  dc.vbat     = dm.vbat;

  //Cruscotto accetta pacchetti da 6 preceduti dal byte 204
  Serial1.write(204);
  Serial1.write((char*)&dc, sizeof(dc)); //vs cruscotto

}

//sezione imu ==================================================================

#include <Wire.h>
#include <SparkFunLSM9DS1.h>

LSM9DS1 imu;

void setupIMU(){
  imu.settings.device.commInterface = IMU_MODE_I2C;
  imu.settings.device.agAddress = 0x6B;
  if (!imu.begin())
  {
    Serial.println("LSM9DS1 not found");
    while(1) ;
  }
}

//sezione daq ==================================================================
struct datiDinamici { //4+2*8+2*6 = 32 byte
  uint32_t t;
  uint16_t a8,a9,a10,a11,a12,a13,a14,a15;
  int16_t ax,ay,az,gx,gy,gz;
} dd;

void daq(){
  dd.a8   = analogRead(A8);
  dd.a9   = analogRead(A9);
  dd.a10  = analogRead(A10);
  dd.a11  = analogRead(A11);
  dd.a12  = analogRead(A12);
  dd.a13  = analogRead(A13);
  dd.a14  = analogRead(A14);
  dd.a15  = analogRead(A15);
  dd.t    = millis();
  imu.readGyro();
  imu.readAccel();
  dd.ax   = imu.ax;
  dd.ay   = imu.ay;
  dd.az   = imu.az;
  dd.gx   = imu.gx;
  dd.gy   = imu.gy;
  dd.gz   = imu.gz;
}

//==============================================================================

void setup()
{
  Serial.begin(115200); //vs raspi
  Serial1.begin(4800); //vs cruscotto
  setupIMU();
  initMotec();

}

int Tcrusc = 100; //periodo in millisecondi tra i frame mandati al cruscotto
int lastcrusc = 0;

int Tdaq = 10;
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
    Serial.write(255);
    Serial.write(255);
    Serial.write((char*)&dm, sizeof(dm));
    Serial.write((char*)&dd, sizeof(dd));
  }


  //se il cruscotto da problemi provare con un ignorantissimo delay(100)
  if (time - lastcrusc >= Tcrusc) {
    lastcrusc = time;
    updateDashboard();
  }

}
