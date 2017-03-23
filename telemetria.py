import datetime
import serial
import struct
arduino = serial.Serial('/dev/cu.usbmodem14121',115200)

print 'rpm\tmap\tair\tlam\ttps\tengt\tbat\toilp\toilt\tgear\tfuel\tvel\tbse\tps2\tpd1\tpd2\n'
while True:
    f=open('{:%Y%m%d%H%M}.csv'.format(datetime.datetime.now()), 'a')
    for i in range (0,600): #600 campioni per file
        while arduino.read() != '\xff':
            pass
        if arduino.read() != '\xff':
            continue
        data = arduino.read(64) #byte grezzi
        val = struct.unpack('<16HL8H6h',data) #valori

        for j in range(0, 16):
            print '{0}\t'.format(val[j]),
        print '\r',

        for el in val:
            f.write("%d;" % el)
        f.write('\n')
