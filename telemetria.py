import socket
import datetime
import can
import struct
import time
import os

bypassCAN = False

leaseLife = 5 #secondi tra verifiche del client
fileSpan = 60 #secondi tra creazione nuovi file
txperiod = 0.10 #secondi tra invii alla telemetria
port = 2017

if bypassCAN == False:
	bus = can.interface.Bus(bustype='socketcan', channel='can0')
	os.system("sudo ip link set can0 up type can bitrate 1000000")
	while True:
		if not bus.recv(0): #svuota il buffer prima di iniziare
			break


s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("", port))
s.settimeout(0)

val=[0]*33
print (val)

cliaddr = None #indirizzo del client (GUI telemetria)

lastCliCheck = 0;
lastfilestart = 0;
lastfilewrite = 0;
lastTx = 0;

while True:
	f = open('{:%Y%m%d%H%M}.csv'.format(datetime.datetime.now()), 'a')
	lastfilestart = time.time();
	while time.time()-lastfilestart < fileSpan: #1 minuto di campioni per file

		#acquisizione da Arduino
		if bypassCAN:
			val = range(1,33)
			time.sleep(0.01)
		else:
			msg = bus.recv(0)
			if msg :
				if msg.arbitration_id == 2:
					val[0:] = struct.unpack('<4H',msg.data)
					print(val)
				#data = b'\x00\x05\x00\x06\x00\x07\x00\x08'
				#val = struct.unpack('<16HL9H6h',data) #formattazione valori
		#fine acquisizione da Arduino

		#salvataggio dati su SD
		if time.time()-lastfilewrite > 0.01:
			for el in val:
				f.write("%d;" % el)
			f.write('\n')
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
