clc
clear
close all

iter = 150;

load("../out/surfacegeo.mat")
load("../out/solution_" + iter + ".mat")

%draw surfaces and tracks
subplot(2,2,1)
axis equal
hold on
grid on

color = ones(1,length(PlaneBoundary_z));
trisurf(PlaneBoundary_T, PlaneBoundary_x, PlaneBoundary_y, PlaneBoundary_z, PlaneBoundary_C, 'LineStyle', 'none');

for i = 1:length(tracks)
    plot3(tracks{i}(:,1), tracks{i}(:,2), tracks{i}(:,3), 'r');
end

view(120, 20);

%draw hits
subplot(2,2,2)
plot(hitsX, hitsY, 'or');
grid on
axis equal
title('Hits')
xlabel('x')
ylabel('y')

%draw charge
subplot(2,2,3)
trisurf(PlaneBoundary_T, PlaneBoundary_x, PlaneBoundary_y, PlaneBoundary_z, PlaneBoundary_C, 'LineStyle', 'none');
view(180, 0);
title('Charge')
xlabel('x')
zlabel('z')
c = colorbar;
c.Label.String = 'charge density';

%draw energy
subplot(2,2,4)
% hist = histogram(energy, 10, 'BinEdges', [0,1e3,2e3,3e3,4e3,5e3,6e3,7e3,8e3,9e3,1e4]);
hist = histogram(energy, 10);
title('Energy')
xlabel('Energy, eV')







