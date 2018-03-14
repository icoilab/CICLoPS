cd v_write/V_Write

fid = fopen('ciclops_doric_invivo_comp.bin');
NumChannels = 1;
channelNumber = 1;
sps = 100;                              %set to sample rate of daq

max = 1;                                %match to min/max input range of daq
min = -1;
m = 1;                                  %calibration factors (optional)
b = 0;

data = m*(max-min)*fread(fid, inf, 'int16')/65536+b;
time = 1/sps:1/sps:size(data(channelNumber:NumChannels:end),1)/sps;
plot(time,data(channelNumber:NumChannels:end));
fclose(fid);

clear fid max min m b ans sps 