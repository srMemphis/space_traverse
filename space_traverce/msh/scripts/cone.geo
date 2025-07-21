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
coneRmin = 0.0003 / 2;          // 0.3 mm
coneRmax = 0.0011 / 2;          // 1.1 mm
coneThiccMax = 0.0005;          // 0.5 mm
coneThiccMin = 0.00015;         // 0.15mm
coneLength = 0.004;             // 4 mm

coneDirX = 0;
coneDirY = 0;
coneDirZ = coneLength;

coneEntranceX = 0;
coneEntranceY = 0;
coneEntranceZ = -coneLength/2; 


Cone(2) = {         // big cone
    coneEntranceX,
    coneEntranceY,
    coneEntranceZ,
    coneDirX,
    coneDirY,
    coneDirZ,
    coneRmax + coneThiccMax,
    coneRmin + coneThiccMin
};

Cone(3) = {         // small cone
    coneEntranceX,
    coneEntranceY,
    coneEntranceZ,
    coneDirX,
    coneDirY,
    coneDirZ,
    coneRmax,
    coneRmin
};

// actual cone
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
MeshSize{ PointsOf{ Volume{4}; } } = 0.00005;
