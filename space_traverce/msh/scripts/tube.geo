// Gmsh project created on Thu Jul 18 14:26:35 2024
SetFactory("OpenCASCADE");

// world box
worldX = 0.2;     // size along x
worldY = 0.2;     // size along y
worldZ = 0.2;     // size along z

Box(1) = {
    -worldX/2,
    -worldY/2,
    -worldZ/2,
    worldX,
    worldY,
    worldZ
};

// glass tube
tubeRmin = 0.001 / 2;
tubeRmax = 0.0015 / 2;
tubeLength = 0.02;

tubeDirX = 0;
tubeDirY = 0;
tubeDirZ = tubeLength;

tubeEntranceX = 0;
tubeEntranceY = 0;
tubeEntranceZ = -tubeLength/2; 


Cylinder(2) = {         // big cylinder
    tubeEntranceX,
    tubeEntranceY,
    tubeEntranceZ,
    tubeDirX,
    tubeDirY,
    tubeDirZ,
    tubeRmax
};

Cylinder(3) = {         // small cylinder
    tubeEntranceX,
    tubeEntranceY,
    tubeEntranceZ,
    tubeDirX,
    tubeDirY,
    tubeDirZ,
    tubeRmin
};

// actual tube
BooleanDifference(4) = {Volume{2}; Delete;} {Volume{3}; Delete;};

// detector box
detX = worldX * 0.9;
detY = worldY * 0.9;
detZ = 0.005;

detCenterX = 0;
detCenterY = 0;
detCenterZ = worldZ/2 - detZ;

Box(5) = {
    -detX/2 + detCenterX,
    -detY/2 + detCenterY,
    -detZ/2 + detCenterZ,
    detX,
    detY,
    detZ
};

// air volume
BooleanDifference(6) = {Volume{1}; Delete;} {Volume{4,5};};

allSurfacesTube[] = Abs(Boundary{ Volume{4}; });
allSurfacesDetector[] = Abs(Boundary{ Volume{5}; });
allSurfacesOfWorld[] = Abs(Boundary{ Volume{6}; });

uniqueSurfacesOfWorld = allSurfacesOfWorld[];
uniqueSurfacesOfWorld -= allSurfacesDetector[];
uniqueSurfacesOfWorld -= allSurfacesTube[];

Physical Volume("World", 1) = {6};
Physical Volume("Tube", 2) = {4};
Physical Volume("Detector", 3) = {5};
Physical Surface("World Boundary", 1) = uniqueSurfacesOfWorld[];
Physical Surface("Tube Boundary", 2) = allSurfacesTube[];

MeshSize{ PointsOf{ Volume{5,6}; } } = 0.007;
MeshSize{ PointsOf{ Volume{4}; } } = 0.0001;
