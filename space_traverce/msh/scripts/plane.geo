// Gmsh project created on Thu Jul 18 14:26:35 2024
SetFactory("OpenCASCADE");

// world box
worldX = 0.3;     // size along x
worldY = 0.3;     // size along y
worldZ = 0.5;     // size along z

Box(1) = {
    -worldX/2,
    -worldY/2,
    -worldZ/2,
    worldX,
    worldY,
    worldZ
};

// glass plane
planeX = 0.04;
planeY = 0.005;
planeZ = 0.1;

planeCenterX = 0;
planeCenterY = 0;
planeCenterZ = 0;

Box(2) = {
    -planeX/2 + planeCenterX,
    -planeY/2 + planeCenterY,
    -planeZ/2 + planeCenterZ,
    planeX,
    planeY,
    planeZ
};

// detector box
detX = worldX * 0.9;
detY = worldY * 0.9;
detZ = 0.01;

detCenterX = 0;
detCenterY = 0;
detCenterZ = worldZ/2 - 0.02;

Box(3) = {
    -detX/2 + detCenterX,
    -detY/2 + detCenterY,
    -detZ/2 + detCenterZ,
    detX,
    detY,
    detZ
};

// world volume
BooleanDifference(4) = {Volume{1}; Delete;} {Volume{2,3};};

PlaneSurfaces = Abs(Boundary{ Volume{2}; });
DetectorSurfaces = Abs(Boundary{ Volume{3}; });
worldSurfaces = Abs(Boundary{ Volume{4}; });

uniqueWorldSurfaces = worldSurfaces[];
uniqueWorldSurfaces-=DetectorSurfaces[];
uniqueWorldSurfaces-=PlaneSurfaces[];

Physical Volume("World", 1) = {4};
Physical Volume("Plane", 2) = {2};
Physical Volume("Detector", 3) = {3};
Physical Surface("World Boundary", 1) = uniqueWorldSurfaces[];
Physical Surface("Plane Boundary", 2) = PlaneSurfaces[];

MeshSize{ PointsOf{ Volume{3,4}; } } = 0.01;
MeshSize{ PointsOf{ Volume{2}; } } = 0.001;
