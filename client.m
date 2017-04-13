t = udp('192.168.0.31',5000);
fopen(t);
t.DatagramTerminateMode='off';
i = 0;
while true
    if mod(i,48)==0
        fwrite(t,'si volaaaa');
    end
%vengono dalla motec
    valrpm=(cast(fread(t,1,'uint16'),'double'))*100;
    valmap=(cast(fread(t,1,'uint16'),'double'))*100;
    vallambda=(cast(fread(t,1,'uint16'),'double'))*283.0189;%1000/5.3*1.5
    valtps=(cast(fread(t,1,'uint16'),'double'))/10;
    valwater=(cast(fread(t,1,'uint16'),'double'));
    valbat=(cast(fread(t,1,'uint16'),'double'))/10;
    valoilpress=(cast(fread(t,1,'uint16'),'double'))/10;
    valoiltemp=(cast(fread(t,1,'uint16'),'double'))/10;
    valgear=(cast(fread(t,1,'uint16'),'double'));
    valbse=(cast(fread(t,1,'uint16'),'double'))/100;
    
%vengono dall'arduino    
    valrearbrake=(cast(fread(t,1,'uint16'),'double'));
    valsuspfl=(cast(fread(t,1,'uint16'),'double'))/10;
    valsuspfr=(cast(fread(t,1,'uint16'),'double'))/10;
    valsusprl=(cast(fread(t,1,'uint16'),'double'))/10;
    valsusprr=(cast(fread(t,1,'uint16'),'double'))/10;
    valspeed=(cast(fread(t,1,'uint16'),'double'))/10;
    valsteer=(cast(fread(t,1,'uint16'),'double'));
    %corrente di luca
    valcurhall=(cast(fread(t,1,'uint16'),'double'));
    valcurhallref=(cast(fread(t,1,'uint16'),'double'));
    valtemppancia=(cast(fread(t,1,'uint16'),'double'));
%vengono dall'accellerometro    
    aznum=(cast(fread(t,1,'int16'),'double'));
    axnum=(cast(fread(t,1,'int16'),'double'));
    aynum=(cast(fread(t,1,'int16'),'double'))  ;  
    i = i + 1;
    disp(i)
end