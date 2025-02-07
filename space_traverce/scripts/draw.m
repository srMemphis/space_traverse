clc
clear
close all

figure()
axis equal
hold on
grid on


load("../surfacegeo.mat")
trisurf(PlaneBoundary_T, PlaneBoundary_x, PlaneBoundary_y, PlaneBoundary_z);


load("../solution.mat")
for i = 1:length(tracks)
    plot3(tracks{i}(:,1), tracks{i}(:,2), tracks{i}(:,3), 'r');
end






