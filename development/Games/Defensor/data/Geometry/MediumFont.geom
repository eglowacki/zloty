// Placeholder file for dynamif font geometry

// vertex formats specify order of Vertex layout
VertexFormat: VertexPosition | VertexColor

Vertices Begin:
    { 0, 0, 0 }; { 0, 0, 0, 0 }
    { 0, 0, 0 }; { 0, 0, 0, 0 }
    { 0, 0, 0 }; { 0, 0, 0, 0 }
Vertices End:

// -1,1     1,1
//   0-------1
//   |\      |
//   |   \   |
//   |      \| 
//   3-------2
// -1,-1    1,-1

Indices Begin:
    0, 1, 2
Indices End:
