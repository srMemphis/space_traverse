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


// mask
maskX = 0.04;
maskY = 0.04;
maskZ = 0.001;

maskCenterX = 0;
maskCenterY = 0;
maskCenterZ = -maskZ/2;

Box(10) = {
    -maskX/2 + maskCenterX,
    -maskY/2 + maskCenterY,
    -maskZ/2 + maskCenterZ,
    maskX,
    maskY,
    maskZ
};

// mask holle
holleD = 0.0009;

Cylinder(11) = {         // big cylinder
    0,
    0,
    0,
    0,
    0,
    -maskZ,
    holleD/2
};

BooleanDifference(12) = {Volume{10}; Delete;} {Volume{11}; Delete;};

// holder
holderX = 0.08;
holderY = 0.01;
holderZ = 0.04;

holderCenterX = 0;
holderCenterY = -0.021 - holderY/2;
holderCenterZ = holderZ/2;

Box(13) = {
    -holderX/2 + holderCenterX,
    -holderY/2 + holderCenterY,
    -holderZ/2 + holderCenterZ,
    holderX,
    holderY,
    holderZ
};


// air volume = world - (cone, detector, mask, holder)
BooleanDifference(14) = {Volume{1}; Delete;} {Volume{8,9,12,13};};

// finish geo
Coherence;

allSurfacesTarget[] = Abs(Boundary{ Volume{8}; });
allSurfacesDetector[] = Abs(Boundary{ Volume{9}; });
allSurfacesMask[] = Abs(Boundary{ Volume{12,13}; });
allSurfacesOfWorld[] = Abs(Boundary{ Volume{14}; });

uniqueSurfacesOfWorld = allSurfacesOfWorld[];
uniqueSurfacesOfWorld -= allSurfacesDetector[];
uniqueSurfacesOfWorld -= allSurfacesTarget[];
uniqueSurfacesOfWorld -= allSurfacesMask[];

Physical Volume("World", 1) = {14};
Physical Volume("Target", 2) = {8};
Physical Volume("Detector", 3) = {9};
Physical Volume("Mask", 4) = {12,13};
Physical Surface("World Boundary", 1) = uniqueSurfacesOfWorld[];
Physical Surface("Target Boundary", 2) = allSurfacesTarget[];
Physical Surface("Mask Boundary", 3) = allSurfacesMask[];

MeshSize{ PointsOf{ Volume{9,14}; } } = 0.007;      // world and detector
MeshSize{ PointsOf{ Volume{12}; } } = 0.003;        // mask
MeshSize{ PointsOf{ Curve{44,48}; } } = 0.0001;     // mask
MeshSize{ PointsOf{ Volume{13}; } } = 0.005;        // holder
MeshSize{ PointsOf{ Volume{8}; } } = 0.00008;       // target
MeshSize{ PointsOf{ Surface{8}; } } = 0.00002;      // target tip
