// CAN Receive Example
//

#include <mcp_can.h>
#include <SPI.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];

#define CAN0_INT 2                              // Set INT to pin 2
MCP_CAN CAN0(53);                               // Set CS to pin 10

struct dati {
  uint16_t id, rpm, map, air, lambda, tps, engtemp, vbat, oilp, oilt, gear, fuel, speed;
} d;
struct crusc {
  uint8_t id, rpm, gear, speed, engtemp, oilp, vbat;
} cr;

void setup()
{
  Serial.begin(115200); //vs raspi
  Serial2.begin(4800); //vs cruscotto
  //init ID costanti per i pacchetti in uscita
  d.id=0xFFFF;
  cr.id=204;
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) != CAN_OK)
    Serial.println("diocane");
  
  CAN0.setMode(MCP_NORMAL);// Set operation mode to normal so the MCP2515 sends acks to received data.

  pinMode(CAN0_INT, INPUT);// Configuring pin for /INT input
}

void loop()
{
  if(!digitalRead(CAN0_INT))// If CAN0_INT pin is low, read receive buffer
  {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);// Read data: len = data length, buf = data byte(s)

    if(rxId==0) return;
    if (len != 8) return; //ogni pacchetto CAN deve avere 8 byte di dati, cioè 4 uint16

    //spacchetto i dati per il mio struct e li metto in scala se necessario
    else if(rxId==2){
      d.rpm=((uint16_t)rxBuf[0] << 8) | rxBuf[1] ; 
      d.map=((uint16_t)rxBuf[2] << 8) | rxBuf[3] ; 
      d.map*=10;
      d.air=((uint16_t)rxBuf[4] << 8) | rxBuf[5] ; 
      d.air*=10;
      d.lambda=((uint16_t)rxBuf[6] << 8) | rxBuf[7] ; 
      d.lambda*=1000;
    } else if (rxId==3){
      d.tps=((uint16_t)rxBuf[0] << 8) | rxBuf[1] ; 
      d.tps*=10;
      d.engtemp=((uint16_t)rxBuf[2] << 8) | rxBuf[3] ; 
      d.engtemp*=10;
      d.vbat=((uint16_t)rxBuf[4] << 8) | rxBuf[5] ; 
      d.vbat*=100;
      d.oilp=((uint16_t)rxBuf[6] << 8) | rxBuf[7] ; 
      d.oilp*=1000;
    } else if (rxId==4){
      d.oilt=((uint16_t)rxBuf[0] << 8) | rxBuf[1] ; 
      d.oilt*=10;
      d.gear=((uint16_t)rxBuf[2] << 8) | rxBuf[3] ; 
      d.fuel=((uint16_t)rxBuf[4] << 8) | rxBuf[5] ; 
      d.speed=((uint16_t)rxBuf[6] << 8) | rxBuf[7] ; 
      d.speed*=1000;

      //dopo che arriva il pacchetto di tipo 4 inoltro i dati:

      //preparo il pacchetto per il cruscotto
      cr.rpm=d.rpm;
      cr.gear=d.gear;
      cr.speed=d.speed;
      cr.engtemp=d.engtemp;
      cr.oilp=d.oilp;
      cr.vbat=d.vbat;
      
      Serial2.write((char*)&cr, 7); //vs cruscotto
      Serial.write((char*)&d, 26); //13*2 vs raspi
    }
  }
}
