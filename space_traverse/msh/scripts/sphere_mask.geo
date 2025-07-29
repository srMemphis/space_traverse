// Gmsh project created on Thu Jul 18 14:26:35 2024
SetFactory("OpenCASCADE");

// world box
Box(1) = {-0.5, -0.5, -0.5, 1, 1, 1};

// Target sphere
r = 0.05;
Sphere(2) = {0, 0, 0, r};

// Grounded mask
Box(3) = {-0.2, -0.2, -0.2, 0.4, 0.05, 0.4};

// Cut everything out of word wolume
BooleanDifference(4) = { Volume{1}; Delete; }{ Volume{2,3};};

// get surfaces
allSurfacesOfWorld[] = Abs(Boundary{ Volume{4}; });
allSurfacesSphere[] = Abs(Boundary{ Volume{2}; });
allSurfacesMask[] = Abs(Boundary{ Volume{3}; });

uniqueSurfacesOfWorld = allSurfacesOfWorld[];
uniqueSurfacesOfWorld -= allSurfacesSphere[];
uniqueSurfacesOfWorld -= allSurfacesMask[];

// create phisical volumes. Traget = sphere
Physical Volume("World", 1) = {4};
Physical Volume("Target", 2) = {2};
Physical Volume("Mask", 3) = {3};
Physical Surface("World Boundary", 1) = uniqueSurfacesOfWorld[];
Physical Surface("Target Boundary", 2) = allSurfacesSphere[];
Physical Surface("Mask Boundary", 3) = allSurfacesMask[];

// Set mesh size
MeshSize{ PointsOf{ Volume{4}; } } = 0.05;
MeshSize{ PointsOf{ Volume{3}; } } = 0.01;
MeshSize{ PointsOf{ Volume{2}; } } = r/10;
