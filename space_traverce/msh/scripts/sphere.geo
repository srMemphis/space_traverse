// Gmsh project created on Thu Jul 18 14:26:35 2024
SetFactory("OpenCASCADE");

// угол бокса и расстояния от него
Box(1) = {-0.5, -0.5, -0.5, 1, 1, 1};

// центр и радиус
r = 0.05;
Sphere(2) = {0, 0, 0, r};

// вырезаем сферу в боксе
v() = BooleanFragments{ Volume{1}; Delete; }{ Volume{2}; Delete; };

Physical Volume("Air", 1) = {3};
Physical Volume("Metall", 2) = {2};
Physical Surface("Air Boundary", 1) = {8:13};
Physical Surface("Metall Boundary", 2) = {7};

MeshSize{ PointsOf{ Volume{3}; } } = 0.01;
MeshSize{ PointsOf{ Volume{2}; } } = r/20;

