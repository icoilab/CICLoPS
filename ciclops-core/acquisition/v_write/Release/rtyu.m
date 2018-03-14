cd 'D:\V_Write\V_Write'

fid = fopen('t7.bin', 'rb');
a = fread(fid, inf, 'int16')/32767;

% sizeb = 8696000;
% b = zeros(sizeb);
% 
% for i = 1:sizeb-1;
%     b[i] = a[i];
% end
%     
fclose(fid);

plot(a)