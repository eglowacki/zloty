#include "Triangle.h"


#if 0
//-------------------------------------------------------------------------------------------------------------
// Vertex Buffer
// Declare Data
struct Vertex
{
    float position[3];
    float color[3];
};

Vertex vertexBufferData[3] =
{
  { { 1.0f,  -1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
  { { -1.0f,  -1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
  { { 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
};

// Declare Handles
ID3D12Resource* vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

const UINT vertexBufferSize = sizeof(vertexBufferData);

D3D12_HEAP_PROPERTIES heapProps;
heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
heapProps.CreationNodeMask = 1;
heapProps.VisibleNodeMask = 1;

D3D12_RESOURCE_DESC vertexBufferResourceDesc;
vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
vertexBufferResourceDesc.Alignment = 0;
vertexBufferResourceDesc.Width = vertexBufferSize;
vertexBufferResourceDesc.Height = 1;
vertexBufferResourceDesc.DepthOrArraySize = 1;
vertexBufferResourceDesc.MipLevels = 1;
vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
vertexBufferResourceDesc.SampleDesc.Count = 1;
vertexBufferResourceDesc.SampleDesc.Quality = 0;
vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

ThrowIfFailed(device->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &vertexBufferResourceDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&vertexBuffer)));

// Copy the triangle data to the vertex buffer.
UINT8* pVertexDataBegin;

// We do not intend to read from this resource on the CPU.
D3D12_RANGE readRange;
readRange.Begin = 0;
readRange.End = 0;

ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
memcpy(pVertexDataBegin, vertexBufferData, sizeof(vertexBufferData));
vertexBuffer->Unmap(0, nullptr);

// Initialize the vertex buffer view.
vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
vertexBufferView.StrideInBytes = sizeof(Vertex);
vertexBufferView.SizeInBytes = vertexBufferSize;


// Declare Data
uint32_t indexBufferData[3] = { 0, 1, 2 };

// Declare Handles
ID3D12Resource* indexBuffer;
D3D12_INDEX_BUFFER_VIEW indexBufferView;

const UINT indexBufferSize = sizeof(indexBufferData);

D3D12_HEAP_PROPERTIES heapProps;
heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
heapProps.CreationNodeMask = 1;
heapProps.VisibleNodeMask = 1;

D3D12_RESOURCE_DESC vertexBufferResourceDesc;
vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
vertexBufferResourceDesc.Alignment = 0;
vertexBufferResourceDesc.Width = indexBufferSize;
vertexBufferResourceDesc.Height = 1;
vertexBufferResourceDesc.DepthOrArraySize = 1;
vertexBufferResourceDesc.MipLevels = 1;
vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
vertexBufferResourceDesc.SampleDesc.Count = 1;
vertexBufferResourceDesc.SampleDesc.Quality = 0;
vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

ThrowIfFailed(device->CreateCommittedResource(
  &heapProps,
  D3D12_HEAP_FLAG_NONE,
  &vertexBufferResourceDesc,
  D3D12_RESOURCE_STATE_GENERIC_READ,
  nullptr,
  IID_PPV_ARGS(&indexBuffer)));

// Copy data to DirectX 12 driver memory:
UINT8* pVertexDataBegin;

D3D12_RANGE readRange;
readRange.Begin = 0;
readRange.End = 0;

ThrowIfFailed(indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
memcpy(pVertexDataBegin, indexBufferData, sizeof(indexBufferData));
indexBuffer->Unmap(0, nullptr);

// Initialize the index buffer view.
indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
indexBufferView.Format = DXGI_FORMAT_R32_UINT;
indexBufferView.SizeInBytes = indexBufferSize;


//-------------------------------------------------------------------------------------------------------------
// Index Buffer
// Declare Data
uint32_t indexBufferData[3] = { 0, 1, 2 };

// Declare Handles
ID3D12Resource* indexBuffer;
D3D12_INDEX_BUFFER_VIEW indexBufferView;

const UINT indexBufferSize = sizeof(indexBufferData);

D3D12_HEAP_PROPERTIES heapProps;
heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
heapProps.CreationNodeMask = 1;
heapProps.VisibleNodeMask = 1;

D3D12_RESOURCE_DESC vertexBufferResourceDesc;
vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
vertexBufferResourceDesc.Alignment = 0;
vertexBufferResourceDesc.Width = indexBufferSize;
vertexBufferResourceDesc.Height = 1;
vertexBufferResourceDesc.DepthOrArraySize = 1;
vertexBufferResourceDesc.MipLevels = 1;
vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
vertexBufferResourceDesc.SampleDesc.Count = 1;
vertexBufferResourceDesc.SampleDesc.Quality = 0;
vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

ThrowIfFailed(device->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &vertexBufferResourceDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&indexBuffer)));

// Copy data to DirectX 12 driver memory:
UINT8* pVertexDataBegin;

D3D12_RANGE readRange;
readRange.Begin = 0;
readRange.End = 0;

ThrowIfFailed(indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
memcpy(pVertexDataBegin, indexBufferData, sizeof(indexBufferData));
indexBuffer->Unmap(0, nullptr);

// Initialize the index buffer view.
indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
indexBufferView.Format = DXGI_FORMAT_R32_UINT;
indexBufferView.SizeInBytes = indexBufferSize;


//-------------------------------------------------------------------------------------------------------------
// Uniform Buffer
// Declare Handles
ID3D12Resource* uniformBuffer;
ID3D12DescriptorHeap* uniformBufferHeap;
UINT8* mappedUniformBuffer;

// Create the UBO

D3D12_HEAP_PROPERTIES heapProps;
heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
heapProps.CreationNodeMask = 1;
heapProps.VisibleNodeMask = 1;

D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
heapDesc.NumDescriptors = 1;
heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&uniformBufferHeap)));

D3D12_RESOURCE_DESC uboResourceDesc;
uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
uboResourceDesc.Alignment = 0;
uboResourceDesc.Width = (sizeof(uboVS) + 255) & ~255;
uboResourceDesc.Height = 1;
uboResourceDesc.DepthOrArraySize = 1;
uboResourceDesc.MipLevels = 1;
uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
uboResourceDesc.SampleDesc.Count = 1;
uboResourceDesc.SampleDesc.Quality = 0;
uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;


ThrowIfFailed(device->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &uboResourceDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&uniformBuffer)));
uniformBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");

D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
cbvDesc.BufferLocation = uniformBuffer->GetGPUVirtualAddress();
cbvDesc.SizeInBytes = (sizeof(uboVS) + 255) & ~255;    // CB size is required to be 256-byte aligned.

D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(uniformBufferHeap->GetCPUDescriptorHandleForHeapStart());
cbvHandle.ptr = cbvHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

device->CreateConstantBufferView(&cbvDesc, cbvHandle);

// We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
D3D12_RANGE readRange;
readRange.Begin = 0;
readRange.End = 0;

ThrowIfFailed(uniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedUniformBuffer)));
memcpy(mappedUniformBuffer, &uboVS, sizeof(uboVS));
uniformBuffer->Unmap(0, &readRange);


//-------------------------------------------------------------------------------------------------------------
// vertex shader
// Declare handles
ID3DBlob* vertexShader = nullptr;
ID3DBlob* errors = nullptr;

#if defined(_DEBUG)
// Enable better shader debugging with the graphics debugging tools.
UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
UINT compileFlags = 0;
#endif
std::string path = "";
char pBuf[1024];

_getcwd(pBuf, 1024);
path = pBuf;
path += "\\";
std::wstring wpath = std::wstring(path.begin(), path.end());

std::wstring vertPath = wpath + L"assets/triangle.vert.hlsl";

try
{
    ThrowIfFailed(D3DCompileFromFile(vertPath.c_str(), nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, &errors));
}
catch (std::exception e)
{
    const char* errStr = (const char*)errors->GetBufferPointer();
    std::cout << errStr;
    errors->Release();
    errors = nullptr;
}


//-------------------------------------------------------------------------------------------------------------
// Pixel Shader
// Declare handles
ID3DBlob* pixelShader = nullptr;
ID3DBlob* errors = nullptr;

#if defined(_DEBUG)
// Enable better shader debugging with the graphics debugging tools.
UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
UINT compileFlags = 0;
#endif
std::string path = "";
char pBuf[1024];

_getcwd(pBuf, 1024);
path = pBuf;
path += "\\";
std::wstring wpath = std::wstring(path.begin(), path.end());

std::wstring fragPath = wpath + L"assets/triangle.frag.hlsl";

try
{
    ThrowIfFailed(D3DCompileFromFile(fragPath.c_str(), nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, &errors));
}
catch (std::exception e)
{
    const char* errStr = (const char*)errors->GetBufferPointer();
    std::cout << errStr;
    errors->Release();
    errors = nullptr;
}


//-------------------------------------------------------------------------------------------------------------
// Pipeline State
// Declare handles
ID3D12PipelineState* pipelineState;

// Define the vertex input layout.
D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
{
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};


D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
psoDesc.pRootSignature = rootSignature;

D3D12_SHADER_BYTECODE vsBytecode;
vsBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
vsBytecode.BytecodeLength = vertexShader->GetBufferSize();

psoDesc.VS = vsBytecode;

D3D12_SHADER_BYTECODE psBytecode;
psBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
psBytecode.BytecodeLength = pixelShader->GetBufferSize();

psoDesc.PS = psBytecode;

D3D12_RASTERIZER_DESC rasterDesc;
rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
rasterDesc.FrontCounterClockwise = FALSE;
rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
rasterDesc.DepthClipEnable = TRUE;
rasterDesc.MultisampleEnable = FALSE;
rasterDesc.AntialiasedLineEnable = FALSE;
rasterDesc.ForcedSampleCount = 0;
rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

psoDesc.RasterizerState = rasterDesc;

D3D12_BLEND_DESC blendDesc;
blendDesc.AlphaToCoverageEnable = FALSE;
blendDesc.IndependentBlendEnable = FALSE;
const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
{
  FALSE,FALSE,
  D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
  D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
  D3D12_LOGIC_OP_NOOP,
  D3D12_COLOR_WRITE_ENABLE_ALL,
};
for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

psoDesc.BlendState = blendDesc;
psoDesc.DepthStencilState.DepthEnable = FALSE;
psoDesc.DepthStencilState.StencilEnable = FALSE;
psoDesc.SampleMask = UINT_MAX;
psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
psoDesc.NumRenderTargets = 1;
psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
psoDesc.SampleDesc.Count = 1;
try
{
    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
}
catch (std::exception e)
{
    std::cout << "Failed to create Graphics Pipeline!";
}


//-------------------------------------------------------------------------------------------------------------
// Render the Triangle
 void render()
{
    // Frame limit set to 60 fps
    tEnd = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
    if (time < (1000.0f / 60.0f))
    {
        return;
    }
    tStart = std::chrono::high_resolution_clock::now();

    // Update Uniforms
    elapsedTime += 0.001f * time;
    elapsedTime = fmodf(elapsedTime, 6.283185307179586f);
    uboVS.modelMatrix = Matrix4::rotationY(elapsedTime);

    D3D12_RANGE readRange;
    readRange.Begin = 0;
    readRange.End = 0;

    ThrowIfFailed(uniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedUniformBuffer)));
    memcpy(mappedUniformBuffer, &uboVS, sizeof(uboVS));
    uniformBuffer->Unmap(0, &readRange);

    setupCommands();

    ID3D12CommandList* ppCommandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present, then wait till finished to continue execution
    swapchain->Present(1, 0);

    const UINT64 fence = fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence, fence));
    fenceValue++;

    if (fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fence, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    frameIndex = swapchain->GetCurrentBackBufferIndex();
}


#endif


yaget::render::rd::Triangle::Triangle()
{
}

yaget::render::rd::Triangle::~Triangle()
{
}
