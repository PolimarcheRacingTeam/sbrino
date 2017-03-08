// CAN Receive Example
//

#include <mcp_can.h>
#include <SPI.h>

#define CAN0_INT 2                              // Set INT to pin 2
MCP_CAN CAN0(53);                               // Set CS to pin 10

struct datiMotec {
  uint16_t rpm, map, air, lambda, tps, engtemp, vbat, oilp, oilt, gear, fuel, speed, bse, tps2, tpd1, tpd2;;
} dm;

long unsigned int rxId;
unsigned char len;
unsigned char rxBuf[8];

//da 1 se è stato ricevuto il pacchetto di tipo 5, altrimenti 0
int getFromMotec() {
  if (!digitalRead(CAN0_INT)) // If CAN0_INT pin is low, read receive buffer
  {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    if (rxId == 0)
      return 0;
    if (len != 8) //ogni pacchetto CAN deve avere 8 byte di dati, cioè 4 uint16
      return 0;

    //spacchetto i dati per il mio struct e li metto in scala se necessario
    else if (rxId == 2) {
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
    } else if (rxId==5){
      d.bse=((uint16_t)rxBuf[0] << 8) | rxBuf[1] ;
      d.bse*=10;
      d.tps2=((uint16_t)rxBuf[2] << 8) | rxBuf[3] ;
      d.tps2*=10;
      d.tpd1=((uint16_t)rxBuf[4] << 8) | rxBuf[5] ;
      d.tpd1*=10;
      d.tpd2=((uint16_t)rxBuf[6] << 8) | rxBuf[7] ;
      d.tpd2*=10;
      return 1;
  }
  return 0;
}

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

void setup()
{
  Serial.begin(115200); //vs raspi
  Serial3.begin(4800); //vs cruscotto

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  while (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) != CAN_OK)
    //finchè non parte il modulo CAN non si fa niente
    delay(10);

  CAN0.setMode(MCP_NORMAL);// Set operation mode to normal so the MCP2515 sends acks to received data.

  pinMode(CAN0_INT, INPUT);// Configuring pin for /INT input
}

int T = 100; //periodo in millisecondi tra i frame mandati al cruscotto
int lastcrusc = 0;
int time;
void loop()
{
  if (getFromMotec()) {
    //invio a raspberry pi (occhio alle tensioni)
    Serial.write(255);
    Serial.write(255);
    Serial.write((char*)&dm, sizeof(dm));
  }
  time = millis();
  if (time - lastcrusc >= T) {
    lastcrusc = time;
    updateDashboard();
  }
}
