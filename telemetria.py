import socket
import datetime
import serial
import struct


debug = True

if debug==False:
    arduino = serial.Serial('/dev/ttyAMA0',115200)
else:
	import time


#print 'rpm\tmap\tair\tlam\ttps\tengt\tbat\toilp\toilt\tgear\tfuel\tvel\tbse\tps2\tpd1\tpd2\n'


port = 5000
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("", port))
s.settimeout(0)
addr = None

while True:
    f=open('{:%Y%m%d%H%M}.csv'.format(datetime.datetime.now()), 'a')
    for i in range (0,6000): #1 minuto di campioni per file

        if debug:
            val = range(1,33)
            time.sleep(0.01)
        else:
	        #lettura da arduino di 66 byte
	        #31 da 2byte e 1 da 4
	        #controllo header
            while arduino.read() != '\xff':
                pass
            if arduino.read() != '\xff':
                continue
            data = arduino.read(66)
            val = struct.unpack('<16HL9H6h',data) #formattazione valori

        #salvataggio dati su SD
        for el in val:
            f.write("%d;" % el)
        f.write('\n')

        #verifica client, ogni 500 campioni (5 secondi circa)
        #per mantenere viva la connessione i client devono mandare almeno
        #un pacchetto ogni 5 secondi
        if i % 500 == 0 :
            #forget client
            addr = None
            #read all incoming data
            dataIsAvailable = True
            while dataIsAvailable:
                try:
                    data, addr = s.recvfrom(255)
                    print addr, ' sent ', data #struct.unpack('B',data)[0]

                except socket.error as msg:
                    #all data has been read
                    #new client is last writer
                    if addr == None:
                        print 'no client'
                    dataIsAvailable = False

        #invio dati
        if addr :
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


        #stampa sul terminale
        # if i % 10 == 0:
        #     for j in range(0, 16):
        #         print '{0}\t'.format(val[j]),
        #     print '\r',

    #endfor
    f.close()
#endwhile
