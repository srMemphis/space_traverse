// Mesh fixture for the unit tests: the unit cube [1, 2]^3, meshed into a few tetrahedra.
// The cube is offset from the origin on purpose, so a bounding box that always contains the origin shows up.
//
// Regenerate cube.msh from this folder with:
//   D:\Programs\gmsh-4.14.0-Windows64\gmsh.exe cube.geo -3 -format msh41 -o cube.msh
SetFactory("OpenCASCADE");

Box(1) = {1, 1, 1, 1, 1, 1};

// element size larger than the cube keeps the tetrahedron count small
Mesh.MeshSizeMin = 2;
Mesh.MeshSizeMax = 2;

eps = 1e-6;
bottom() = Surface In BoundingBox{1 - eps, 1 - eps, 1 - eps, 2 + eps, 2 + eps, 1 + eps};
top() = Surface In BoundingBox{1 - eps, 1 - eps, 2 - eps, 2 + eps, 2 + eps, 2 + eps};

Physical Volume("Cube") = {1};
Physical Surface("Bottom") = bottom();
Physical Surface("Top") = top();
