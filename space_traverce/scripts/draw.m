clc
clear
close all

figure()
axis equal
hold on
grid on

iter = 999;

load("../out/surfacegeo.mat")
load("../out/solution_" + iter + ".mat")

%draw surfaces
color = ones(1,length(PlaneBoundary_z));
trisurf(PlaneBoundary_T, PlaneBoundary_x, PlaneBoundary_y, PlaneBoundary_z, PlaneBoundary_C, 'LineStyle', 'none');
clb=colorbar;
clb.Label.String = 'Charge density';

%draw tracks
for i = 1:length(tracks)
    plot3(tracks{i}(:,1), tracks{i}(:,2), tracks{i}(:,3), 'r');
end
