//! Names for the watched methods.

/// IDirect3DDevice9, in vtable order.
pub const DEVICE: [&str; 119] = [
    "QueryInterface", "AddRef", "Release", "TestCooperativeLevel", "GetAvailableTextureMem",
    "EvictManagedResources", "GetDirect3D", "GetDeviceCaps", "GetDisplayMode", "GetCreationParameters",
    "SetCursorProperties", "SetCursorPosition", "ShowCursor", "CreateAdditionalSwapChain", "GetSwapChain",
    "GetNumberOfSwapChains", "Reset", "Present", "GetBackBuffer", "GetRasterStatus",
    "SetDialogBoxMode", "SetGammaRamp", "GetGammaRamp", "CreateTexture", "CreateVolumeTexture",
    "CreateCubeTexture", "CreateVertexBuffer", "CreateIndexBuffer", "CreateRenderTarget", "CreateDepthStencilSurface",
    "UpdateSurface", "UpdateTexture", "GetRenderTargetData", "GetFrontBufferData", "StretchRect",
    "ColorFill", "CreateOffscreenPlainSurface", "SetRenderTarget", "GetRenderTarget", "SetDepthStencilSurface",
    "GetDepthStencilSurface", "BeginScene", "EndScene", "Clear", "SetTransform",
    "GetTransform", "MultiplyTransform", "SetViewport", "GetViewport", "SetMaterial",
    "GetMaterial", "SetLight", "GetLight", "LightEnable", "GetLightEnable",
    "SetClipPlane", "GetClipPlane", "SetRenderState", "GetRenderState", "CreateStateBlock",
    "BeginStateBlock", "EndStateBlock", "SetClipStatus", "GetClipStatus", "GetTexture",
    "SetTexture", "GetTextureStageState", "SetTextureStageState", "GetSamplerState", "SetSamplerState",
    "ValidateDevice", "SetPaletteEntries", "GetPaletteEntries", "SetCurrentTexturePalette", "GetCurrentTexturePalette",
    "SetScissorRect", "GetScissorRect", "SetSoftwareVertexProcessing", "GetSoftwareVertexProcessing", "SetNPatchMode",
    "GetNPatchMode", "DrawPrimitive", "DrawIndexedPrimitive", "DrawPrimitiveUP", "DrawIndexedPrimitiveUP",
    "ProcessVertices", "CreateVertexDeclaration", "SetVertexDeclaration", "GetVertexDeclaration", "SetFVF",
    "GetFVF", "CreateVertexShader", "SetVertexShader", "GetVertexShader", "SetVertexShaderConstantF",
    "GetVertexShaderConstantF", "SetVertexShaderConstantI", "GetVertexShaderConstantI", "SetVertexShaderConstantB", "GetVertexShaderConstantB",
    "SetStreamSource", "GetStreamSource", "SetStreamSourceFreq", "GetStreamSourceFreq", "SetIndices",
    "GetIndices", "CreatePixelShader", "SetPixelShader", "GetPixelShader", "SetPixelShaderConstantF",
    "GetPixelShaderConstantF", "SetPixelShaderConstantI", "GetPixelShaderConstantI", "SetPixelShaderConstantB", "GetPixelShaderConstantB",
    "DrawRectPatch", "DrawTriPatch", "DeletePatch", "CreateQuery",
];

/// BullyNiDX9Renderer's methods, named from the arguments they get in the game
/// (stage 2 capture) where that settles it.
pub fn renderer(i: usize) -> String {
    let known = match i {
        2 => "GetRTTI",
        18 => "GetDriverInfo?",
        22 => "? (NiColorA*, 1.0, 1.0)",
        29 => "GetDefaultRenderTargetGroup? ([this+0x680])",
        30 => "? ([this+0x684])",
        42 => "PrecacheGeometry? (NiGeometry*)",
        46 => "? (NiGeometry*)",
        47 => "? (6|8, 1, NiGeometry*|shader data)",
        48 => "Purge/PrecacheGeometryData (NiGeometryData*)",
        49 => "PurgeMaterial (NiMaterialProperty*)",
        50 => "PurgeEffect (NiTextureEffect*)",
        58 => "FastCopy (Ni2DBuffer*, Ni2DBuffer*)",
        67 => "texture renderer data (NiSourceTexture*)",
        76 => "shader versions (vs_3_0/ps_3_0)",
        80 | 81 => "no-op pass marker",
        82 => "BeginFrame",
        83 => "EndFrame",
        84 => "DisplayFrame",
        85 => "ClearBuffer (NiRect*, mode)",
        86 => "SetCameraData (eye, dir, up, right, ...)",
        87 => "SetScreenSpaceCameraData (NiRect*)",
        88 => "GetCameraData (out params)",
        89 => "BeginUsingRenderTargetGroup (group, clear mode)",
        90 => "EndUsingRenderTargetGroup",
        95 => "RenderShape (NiTriShape*)",
        96 => "RenderTristrips (NiTriStrips*)",
        100 => "DrawMesh (NiGeometry*, data, skin=0, NiTransform*)",
        101 => "DrawSkinnedMesh (NiGeometry*, data, NiSkinInstance*)",
        _ => "",
    };
    if known.is_empty() { format!("slot {i} (+{:#05x})", i * 4) } else { format!("{known} (+{:#05x})", i * 4) }
}
