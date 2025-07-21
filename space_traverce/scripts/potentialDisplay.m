clc
clear
close all



%open files
path = "../out/capillary0.0degMultiple";
iter = 100;
load(path + "/solution_" + iter + ".mat", "u");
load(path + "/surfacegeo.mat", "node*");
load(path + "/surfacegeo.mat", "TargetBoundary*");

%generade grid
gridsize = 300;

% maxX = max(nodeX);
% maxY = max(nodeY);
% maxZ = max(nodeZ);
% 
% minX = min(nodeX);
% minY = min(nodeY);
% minZ = min(nodeZ);

maxX = max(nodeX)/10;
maxY = max(nodeY)/10;
maxZ = max(nodeZ)/5 + 0.005;

minX = min(nodeX)/10;
minY = min(nodeY)/10;
minZ = min(nodeZ)/5 + 0.005;


[X,Y,Z] = meshgrid(linspace(minX, maxX, gridsize),linspace(minY, maxY, gridsize),linspace(minZ, maxZ, gridsize));

U = griddata(nodeX, nodeY, nodeZ, u, X, Y, Z);

%plot slices
xslice = 0;
yslice = 0;
zslice = [];
planes = slice(X,Y,Z,U,xslice,yslice,zslice);
set(planes,'edgecolor','none')

xlabel("x");
ylabel("y");
zlabel("z");

%draw body
hold on
trisurf(TargetBoundary_T, TargetBoundary_x, TargetBoundary_y, TargetBoundary_z, 'FaceColor', 'k', 'LineStyle', 'none', 'FaceAlpha', 0.2);

%plot limits
xlim([minX, maxX]);
ylim([minY, maxY]);
zlim([minZ, maxZ]);

