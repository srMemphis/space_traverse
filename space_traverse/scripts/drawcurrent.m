clc
clear
close all


startiter = 0;
stopiter = 2000;
step = 100;

inputCurrent = 100e-9;

path = "../out/capillary0.0degMultiple";


totcurrent = 0;
iter = 0;

count = 1;

for i = startiter:step:stopiter
    load(path + "/solution_" + i + ".mat", "current")
    
    totcurrent(count) = -current / inputCurrent;
    iter(count) = i;
    
    count = count + 1;
end

plot(iter, totcurrent)
grid on
xlabel('Итерация')
ylabel('Доля прошедших электронов')




