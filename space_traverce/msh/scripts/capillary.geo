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

// capillary paramters
capillaryLength = 0.015;        // 15 мм
capillaryOutlet = 0.0003;       // 0.3 mm
capillaryInlet = 0.0011;        // 1.1 mm
capillaryTipLength = 0.004;     // 4 mm
capillaryWallThicMax = 0.0005;  // 0.5 mm
capillaryWallThicMin = 0.00015; // 0.15 mm

// cone
coneDirX = 0;
coneDirY = 0;
coneDirZ = capillaryTipLength;

coneEntranceX = 0;
coneEntranceY = 0;
coneEntranceZ = capillaryLength - capillaryTipLength; 


Cone(2) = {         // big cone
    coneEntranceX,
    coneEntranceY,
    coneEntranceZ,
    coneDirX,
    coneDirY,
    coneDirZ,
    (capillaryInlet + capillaryWallThicMax)/2,
    (capillaryOutlet + capillaryWallThicMin)/2
};

Cone(3) = {         // small cone
    coneEntranceX,
    coneEntranceY,
    coneEntranceZ,
    coneDirX,
    coneDirY,
    coneDirZ,
    capillaryInlet/2,
    capillaryOutlet/2
};

// actual cone
BooleanDifference(4) = {Volume{2}; Delete;} {Volume{3}; Delete;};

// tube part

Cylinder(5) = {         // big cylinder
    0,
    0,
    0,
    0,
    0,
    capillaryLength - capillaryTipLength,
    (capillaryInlet + capillaryWallThicMax)/2
};

Cylinder(6) = {         // small cylinder
    0,
    0,
    0,
    0,
    0,
    capillaryLength - capillaryTipLength,
    capillaryInlet/2
};

// actual tube
BooleanDifference(7) = {Volume{5}; Delete;} {Volume{6}; Delete;};

// adding cone and tube to form capillary
BooleanUnion(8) = {Volume{4}; Delete;} {Volume{7}; Delete;};

// detector box
detX = worldX * 0.9;
detY = worldY * 0.9;
detZ = 0.005;

detCenterX = 0;
detCenterY = 0;
detCenterZ = worldZ/2 - detZ;

Box(9) = {
    -detX/2 + detCenterX,
    -detY/2 + detCenterY,
    -detZ/2 + detCenterZ,
    detX,
    detY,
    detZ
};

// air volume = world - (cone, detector)
BooleanDifference(10) = {Volume{1}; Delete;} {Volume{8,9};};

allSurfacesTarget[] = Abs(Boundary{ Volume{8}; });
allSurfacesDetector[] = Abs(Boundary{ Volume{9}; });
allSurfacesOfWorld[] = Abs(Boundary{ Volume{10}; });

uniqueSurfacesOfWorld = allSurfacesOfWorld[];
uniqueSurfacesOfWorld -= allSurfacesDetector[];
uniqueSurfacesOfWorld -= allSurfacesTarget[];

Physical Volume("World", 1) = {10};
Physical Volume("Target", 2) = {8};
Physical Volume("Detector", 3) = {9};
Physical Surface("World Boundary", 1) = uniqueSurfacesOfWorld[];
Physical Surface("Target Boundary", 2) = allSurfacesTarget[];

MeshSize{ PointsOf{ Volume{9,10}; } } = 0.007;
MeshSize{ PointsOf{ Volume{8}; } } = 0.00008;
MeshSize{ PointsOf{ Surface{8}; } } = 0.00002;
