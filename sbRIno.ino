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

void initCan(){
  // Initialize MCP2515 running at 8Mhz with a baudrate of 1000kb/s and the
  //masks and filters disabled.
  while (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) != CAN_OK){
    //finchè non parte il modulo CAN non si fa niente
    Serial.println("MCP2515 not found");
    delay(1000);
  }
  CAN0.setMode(MCP_NORMAL);// Set operation mode to normal so the MCP2515 sends
  //acks to received data.

  pinMode(CAN0_INT, INPUT);// Configuring pin for /INT input
}
//da 1 se è stato ricevuto il pacchetto di tipo 5, altrimenti 0
int getFromMotec() {
  if (!digitalRead(CAN0_INT)) // If CAN0_INT pin is low, read receive buffer
  {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    if (rxId != 0 && len == 8) { //ogni pacchetto CAN deve avere 8 byte di dati,
      // cioè 4 uint16

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
  Serial3.write(204);
  Serial3.write((char*)&dc, sizeof(dc)); //vs cruscotto

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
  imu.setAccelScale(4); //+-4g

}

//sezione daq ==================================================================
struct datiDinamici { //4+2*9+2*6 = 34 byte
  uint32_t t;
  uint16_t a5,a6,a7,a9,a10,a11,a12,a13,a14;
  int16_t ax,ay,az,gx,gy,gz;
} dd;

void daq(){
  dd.a5   = 0;//analogRead(A5);
  dd.a6   = analogRead(A6); //ntc
  dd.a7   = 0;//analogRead(A7);

  dd.a9   = analogRead(A0); //rear brake
  dd.a10  = analogRead(A1);
  dd.a11  = analogRead(A2);
  dd.a12  = analogRead(A3);
  dd.a13  = analogRead(A4);
  dd.a14  = analogRead(A5); //steer not connected

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
//INVIO DATI DINAMICI SUL CAN
void txDynamic(){

  //CAN0.sendMsgBuf(0x100, 0, 8, data);
}


//=============================================================================

//MAIN


void setup()
{
  Serial.begin(115200); //vs usb per debugging
  Serial3.begin(4800); //vs cruscotto
  setupIMU();
  initCan();
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
    txDynamic();
  }

  if (time - lastcrusc >= Tcrusc) {
    lastcrusc = time;
    updateDashboard();
  }

}
