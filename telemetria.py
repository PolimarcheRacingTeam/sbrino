import socket
import datetime
import serial
import struct

bypassSerial = False
printData = False

arduinoHeaderLow = 123
arduinoHeaderHigh = 234

leaseLife = 500 #numero di campioni tra verifiche dei client
port = 5000

if bypassSerial==True:
    import time
else:
    arduino = serial.Serial('/dev/ttyAMA0',115200)

if printData==True:
    print  ('rpm\tmap\tair\tlam\ttps\tengt\tbat\toilp\t'
            'oilt\tgear\tfuel\tvel\tbse\tps2\tpd1\tpd2\n')

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("", port))
s.settimeout(0)

cliaddr = None #indirizzo del client (GUI telemetria)

while True:
    f=open('{:%Y%m%d%H%M}.csv'.format(datetime.datetime.now()), 'a')
    for i in range (0,6000): #1 minuto di campioni per file

        #acquisizione da Arduino
        if bypassSerial:
            val = range(1,33)
            time.sleep(0.01)
        else:
	        #lettura da arduino di 66 byte
	        #31 da 2byte e 1 da 4
	        #controllo header da 2 byte:
            while True:
                if arduino.read() != arduinoHeaderLow:
                    continue
                if arduino.read() == arduinoHeaderHigh:
                    break

            data = arduino.read(66)
            val = struct.unpack('<16HL9H6h',data) #formattazione valori
        #fine acquisizione da Arduino

        #salvataggio dati su SD
        for el in val:
            f.write("%d;" % el)
        f.write('\n')
        #fine salvataggio dati su SD

        #verifica client, ogni 500 campioni (5 secondi circa)
        #per mantenere viva la connessione i client devono mandare almeno
        #un pacchetto ogni 5 secondi
        if i % leaseLife == 0 :
            #forget client
            cliaddr = None
            #read all incoming data
            dataIsAvailable = True
            while dataIsAvailable:
                try:
                    data, cliaddr = s.recvfrom(255)
                    print cliaddr, ' sent ', data #struct.unpack('B',data)[0]

                except socket.error as msg:
                    #all data has been read
                    #new client is last writer
                    if cliaddr == None:
                        print 'no client'
                    dataIsAvailable = False
        #fine verifica client

        #invio dati a client
        if cliaddr :
            if i % 10 == 0:
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
                    val[21],
                    val[22],
                    val[23],
                    val[24],
                    val[11], #speed
                    val[25], #steer
                    val[17],
                    val[18],
                    val[19],
                    val[28], #az
                    val[26], #ax
                    val[27]  #ay
                )
                s.sendto(packet, addr)
        #fine invio dati a client

        #stampa dati sul terminale
        if printData==True:
            if i % 10 == 0:
                for j in range(0, 16):
                    print '{0}\t'.format(val[j]),
                print '\r',
        #fine stampa dati sul terminale
    #fine file da 6000 campioni
    f.close()
#fine loop infinito
