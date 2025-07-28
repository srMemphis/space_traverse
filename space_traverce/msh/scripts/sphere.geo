// Gmsh project created on Thu Jul 18 14:26:35 2024
SetFactory("OpenCASCADE");

// угол бокса и расстояния от него
Box(1) = {-0.5, -0.5, -0.5, 1, 1, 1};

// центр и радиус
r = 0.05;
Sphere(2) = {0, 0, 0, r};

// вырезаем сферу в боксе
BooleanDifference(3) = { Volume{1}; Delete; }{ Volume{2};};

// get surfaces
allSurfacesOfWorld[] = Abs(Boundary{ Volume{3}; });
allSurfacesSphere[] = Abs(Boundary{ Volume{2}; });

uniqueSurfacesOfWorld = allSurfacesOfWorld[];
uniqueSurfacesOfWorld -= allSurfacesSphere[];

// create phisical volumes. Traget = sphere
Physical Volume("World", 1) = {3};
Physical Volume("Target", 2) = {2};
Physical Surface("Target Boundary", 1) = uniqueSurfacesOfWorld[];
Physical Surface("Sphere Boundary", 2) = allSurfacesSphere[];

// Set mesh size
MeshSize{ PointsOf{ Volume{3}; } } = 0.05;
MeshSize{ PointsOf{ Volume{2}; } } = r/10;
