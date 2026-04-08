// This represnets full screen quad

// vertex formats specify order of Vertex layout
VertexFormat: VertexPosition | VertexColor | VertexTexture0

Vertices Begin:
    { -1.0f,  1.0f, 0.0f }; { 1.0f, 0.0f, 0.0f, 1.0f }; { 0.0f, 0.0f }
    {  1.0f,  1.0f, 0.0f }; { 0.0f, 1.0f, 0.0f, 1.0f }; { 1.0f, 0.0f }
    {  1.0f, -1.0f, 0.0f }; { 0.0f, 0.0f, 1.0f, 1.0f }; { 1.0f, 1.0f }
    { -1.0f, -1.0f, 0.0f }; { 0.0f, 0.0f, 1.0f, 1.0f }; { 0.0f, 1.0f }
Vertices End:

//  0-------1
//  |\      |
//  |   \   |
//  |      \| 
//  3-------2

Indices Begin:
    0, 1, 2
    0, 2, 3
Indices End:
