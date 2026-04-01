// Simple geometry definition file. March 29, 2026

// vertex formats specify order of Vertex layout
VertexFormat: VertexPosition | VertexColor | VertexTexture0

Vertices Begin:
    { -1.0f,  1.0f , 0.0f }; { 1.0f, 0.0f, 0.0f, 0.99f }; { 0.0f, 0.0f }
    {  1.0f,  1.0f , 0.0f }; { 0.0f, 1.0f, 0.0f, 0.0f };  { 1.0f, 0.0f }
    { -1.0f, -1.0f , 0.0f }; { 0.0f, 0.0f, 1.0f, 0.0f };  { 0.0f, 1.0f }

    //{  1.0f,  1.0f , 0.0f }; { 0.0f, 1.0f, 0.0f, 0.0f }; { 1.0f, 0.0f }
    {  1.0f, -1.0f , 0.0f }; { 0.0f, 0.0f, 1.0f, 0.0f };  { 1.0f, 1.0f }
    //{ -1.0f, -1.0f , 0.0f }; { 0.0f, 0.0f, 1.0f, 0.0f };  { 0.0f, 1.0f }
Vertices End:


Indices Begin:
    0, 1, 2
    1, 3, 2
Indices End:
