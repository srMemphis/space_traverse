clc
clear
close all

iter = 0;
load("../out/capillary_holder/surfacegeo.mat")
load("../out/capillary_holder/solution_" + iter + ".mat")

%draw surfaces and tracks
subplot(2,2,1)
axis equal
hold on
grid on

color = ones(1,length(TargetBoundary_z));
trisurf(TargetBoundary_T, TargetBoundary_x, TargetBoundary_y, TargetBoundary_z, TargetBoundary_C, 'LineStyle', 'none', 'FaceAlpha', 0.25);

% color = ones(1,length(TubeBoundary_z));
% trisurf(TubeBoundary_T, TubeBoundary_x, TubeBoundary_y, TubeBoundary_z, TubeBoundary_C, 'LineStyle', 'none', 'FaceAlpha', 0.25);

for i = 1:length(tracks)
    plot3(tracks{i}(:,1), tracks{i}(:,2), tracks{i}(:,3), 'r');
end

% zlim([-0.02, 0.02])

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

% trisurf(TubeBoundary_T, TubeBoundary_x, TubeBoundary_y, TubeBoundary_z, TubeBoundary_C, 'LineStyle', 'none', 'FaceAlpha', 0.25);
colormap('jet')
trisurf(TargetBoundary_T, TargetBoundary_x, TargetBoundary_y, TargetBoundary_z, TargetBoundary_C, 'LineStyle', 'none', 'FaceAlpha', 0.25);
view(180, 0);
title('Charge')
xlabel('x')
zlabel('z')
c = colorbar;
c.Label.String = 'charge density';

%draw energy
subplot(2,2,4)
% hist = histogram(energy, 10, 'BinEdges', [0,1e3,2e3,3e3,4e3,5e3,6e3,7e3,8e3,9e3,1e4]);
% hist = histogram(energy, 10);
% title('Energy')
% xlabel('Energy, eV')







min(u)





figure()
hold on
axis equal
grid on 
colormap('jet')
color = ones(1,length(TargetBoundary_z));
trisurf(TargetBoundary_T, TargetBoundary_x, TargetBoundary_y, TargetBoundary_z, TargetBoundary_C, 'LineStyle', 'none', 'FaceAlpha', 0.25);


% color = ones(1,length(TubeBoundary_z));
% trisurf(TubeBoundary_T, TubeBoundary_x, TubeBoundary_y, TubeBoundary_z, TubeBoundary_C, 'LineStyle', 'none', 'FaceAlpha', 0.25);

for i = 1:length(tracks)
    plot3(tracks{i}(:,1), tracks{i}(:,2), tracks{i}(:,3), 'b');
end

zlim([-0.005, 0.02])
xlim([-0.002, 0.002]);
ylim([-0.002, 0.002]);

view([1,1,0]);

camup([1,0,0])
camup('manual')






figure()
hold on
axis equal
grid on 
colormap('jet')
c = colorbar
c.Label.String = 'charge density';
color = ones(1,length(TargetBoundary_z));
trisurf(TargetBoundary_T, TargetBoundary_x, TargetBoundary_y, TargetBoundary_z, TargetBoundary_C, 'LineStyle', 'none', 'FaceAlpha', 0.25);
zlim([0.01, 0.016])
xlim([-0.001, 0.001]);
ylim([-0.001, 0.001]);

view([0,1,0]);

camup([1,0,0])
camup('manual')

