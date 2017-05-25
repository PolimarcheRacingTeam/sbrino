import socket
import datetime
import can
import struct
import time
import os

#struttura che contiene tutti i dati
class cardata:
  def __init__(self):
    self.rpm = 0
    self.map = 0
    self.air = 0
    self.lam = 0
    self.tps = 0
    self.eng = 0
    self.bat = 0
    self.oilp = 0
    self.oilt = 0
    self.gear = 0
    self.fuel = 0
    self.vel = 0
    self.bse = 0
    self.tps2 = 0
    self.tpd1 = 0
    self.tpd2 = 0
    self.millis = 0
    self.a5 = 0
    self.ntc = 0
    self.a7 = 0
    self.rearbrake = 0
    self.fr = 0
    self.fl = 0
    self.rr = 0
    self.rl = 0
    self.steer = 0
    self.ax = 0
    self.ay = 0
    self.az = 0
    self.gx = 0
    self.gy = 0
    self.gz = 0

  def iter(self):
    yield self.rpm
    yield self.map
    yield self.air
    yield self.lam
    yield self.tps
    yield self.eng
    yield self.bat
    yield self.oilp
    yield self.oilt
    yield self.gear
    yield self.fuel
    yield self.vel
    yield self.bse
    yield self.tps2
    yield self.tpd1
    yield self.tpd2
    yield self.millis
    yield self.a5
    yield self.ntc
    yield self.a7
    yield self.rearbrake
    yield self.fr
    yield self.fl
    yield self.rr
    yield self.rl
    yield self.steer
    yield self.ax
    yield self.ay
    yield self.az
    yield self.gx
    yield self.gy
    yield self.gz
    raise StopIteration

d=cardata()

bypassCAN = False

leaseLife = 5 #secondi tra verifiche del client
fileSpan = 60 #secondi tra creazione nuovi file
txperiod = 0.10 #secondi tra invii alla telemetria
port = 2017

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("", port))
s.settimeout(0)

if bypassCAN == False:
  bus = can.interface.Bus(bustype='socketcan', channel='can0')
  os.system("sudo ip link set can0 up type can bitrate 1000000")
  while False:
    if not bus.recv(0): #svuota il buffer prima di iniziare
      break

cliaddr = None #indirizzo del client (GUI telemetria)

lastCliCheck = 0;
lastfilestart = 0;
lastfilewrite = 0;
lastTx = 0;

while True:
  print("new file")
  f = open('{:%Y%m%d%H%M}.csv'.format(datetime.datetime.now()), 'a')
  lastfilestart = time.time();
  while time.time()-lastfilestart < fileSpan: #1 minuto di campioni per file

    #acquisizione da Arduino
    while True:
      msg = bus.recv(0)
      if msg :
        if msg.arbitration_id == 2:
          d.rpm, d.map, d.air, d.lam = struct.unpack('>4H',msg.data)
        elif msg.arbitration_id == 3:
          d.tps, d.eng, d.bat, d.oilp = struct.unpack('>4H',msg.data)
        elif msg.arbitration_id == 4:
          d.oilp, d.gear, d.fuel, d.vel = struct.unpack('>4H',msg.data)
        elif msg.arbitration_id == 5:
          d.bse, d.tps2, d.tpd1, d.tpd2 = struct.unpack('>4H',msg.data)
        elif msg.arbitration_id == 10:
          d.millis = struct.unpack('>L',msg.data)
        elif msg.arbitration_id == 11:
          d.a5, d.ntc, d.a7, d.rearbrake = struct.unpack('>4H',msg.data)
        elif msg.arbitration_id == 12:
          d.fr, d.fl, d.rr, d.rl = struct.unpack('>4H',msg.data)
        elif msg.arbitration_id == 13:
          d.steer, d.ax, d.ay, d.az = struct.unpack('>4H',msg.data)
        elif msg.arbitration_id == 14:
          d.gx, d.gy, d.gz = struct.unpack('>3H',msg.data)
      else :
        break
    #fine acquisizione da Arduino

    #salvataggio dati su SD
    if time.time()-lastfilewrite > 0.01:
      for el in d.iter():
        f.write("%d;" % el)
      f.write('\n')
      lastfilewrite=time.time()
    #fine salvataggio dati su SD

    #verifica client, ogni 500 campioni (5 secondi circa)
    #per mantenere viva la connessione i client devono mandare almeno
    #un pacchetto ogni 5 secondi
    if time.time()-lastCliCheck > leaseLife :
      lastCliCheck =  time.time()
      #forget client
      cliaddr = None
      #read all incoming data
      dataIsAvailable = True
      while dataIsAvailable:
        try:
          data, cliaddr = s.recvfrom(255)
          print (cliaddr, ' sent ', data) #struct.unpack('B',data)[0]

        except socket.error as msg:
          #all data has been read
          #new client is last writer
          if cliaddr == None:
            print ('no client')
          dataIsAvailable = False
    #fine verifica client

    #invio dati a client
    if cliaddr :
      if time.time()-lastTx > txperiod:
        lastTx=time.time()
        packet = struct.pack('>20H3h',
          val[0], #rpm
          val[1], #map
          val[3], #lam
          val[4], #tps
          val[5], #water temp
          val[6], #vbat
          val[7], #oil press
          val[8], #oil temp
          val[9], #gear
          val[12], #bse
          val[20], #rear brake
          val[21], #fr
          val[22], #fl
          val[23], #rr
          val[24], #rl
          val[11], #speed
          val[25], #steer
          val[17],
          val[18], #ntc
          val[19],
          val[28], #az
          val[26], #ax
          val[27]  #ay
        )
        s.sendto(packet, addr)
    #fine invio dati a client

  #fine file
  f.close()
#fine loop infinito
