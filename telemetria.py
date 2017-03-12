import datetime
import serial
import struct
arduino = serial.Serial('/dev/ttyACM0',115200)

print 'rpm\tmap\tair\tlam\ttps\tengt\tbat\toilp\toilt\tgear\tfuel\tvel\bse\tps2\tpd1\tpd2\n'
while True:
    f=open('{:%Y%m%d%H%M}.csv'.format(datetime.datetime.now()), 'a')
    for i in range (0,600): #600 campioni per file
        while arduino.read() != '\xff':
            pass
        if arduino.read() != '\xff':
            continue
        data = arduino.read(24)
        val = struct.unpack('<12H',data)

        for j in range(0, 16):
            print '{0}\t'.format(val[j]),
        print '\r',

        for j in range(0, 16):
            f.write("%d;" % val[j])
        f.write('/n')
