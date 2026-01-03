# DirectXTK for Linux Cross-Compilation
# This file handles fetching and building DirectXTK/DirectXTK12 with shader stubs
# for cross-compiling Windows binaries on Linux

include(FetchContent)

# Disable features that require WinRT or special SDK not available in cross-compilation
set(BUILD_TOOLS OFF CACHE BOOL "Disable XWBTool" FORCE)
set(BUILD_XAUDIO_WIN10 OFF CACHE BOOL "Disable XAudio 2.9" FORCE)
set(BUILD_XAUDIO_WIN8 OFF CACHE BOOL "Disable XAudio 2.8" FORCE)
set(BUILD_XAUDIO_WIN7 OFF CACHE BOOL "Disable XAudio 2.7" FORCE)
set(BUILD_GAMEINPUT OFF CACHE BOOL "Disable GameInput" FORCE)
set(BUILD_WGI OFF CACHE BOOL "Disable Windows.Gaming.Input" FORCE)
set(BUILD_XINPUT ON CACHE BOOL "Use XInput" FORCE)

# Shader stub generation function
# These are placeholder shaders that allow building but won't work at runtime
# Real shaders must be compiled on Windows using: Src/Shaders/CompileShaders.cmd
function(_generate_shader_stubs SHADER_DIR)
    message(STATUS "Generating DirectXTK shader stubs in ${SHADER_DIR}...")
    
    set(SHADER_NAMES
        # AlphaTestEffect
        AlphaTestEffect_PSAlphaTestEqNe AlphaTestEffect_PSAlphaTestEqNeNoFog
        AlphaTestEffect_PSAlphaTestLtGt AlphaTestEffect_PSAlphaTestLtGtNoFog
        AlphaTestEffect_VSAlphaTest AlphaTestEffect_VSAlphaTestNoFog
        AlphaTestEffect_VSAlphaTestVc AlphaTestEffect_VSAlphaTestVcNoFog
        # BasicEffect
        BasicEffect_PSBasic BasicEffect_PSBasicNoFog BasicEffect_PSBasicPixelLighting
        BasicEffect_PSBasicPixelLightingTx BasicEffect_PSBasicTx BasicEffect_PSBasicTxNoFog
        BasicEffect_PSBasicVertexLighting BasicEffect_PSBasicVertexLightingNoFog
        BasicEffect_PSBasicVertexLightingTx BasicEffect_PSBasicVertexLightingTxNoFog
        BasicEffect_VSBasic BasicEffect_VSBasicNoFog BasicEffect_VSBasicOneLight
        BasicEffect_VSBasicOneLightBn BasicEffect_VSBasicOneLightTx BasicEffect_VSBasicOneLightTxBn
        BasicEffect_VSBasicOneLightTxVc BasicEffect_VSBasicOneLightTxVcBn BasicEffect_VSBasicOneLightVc
        BasicEffect_VSBasicOneLightVcBn BasicEffect_VSBasicPixelLighting BasicEffect_VSBasicPixelLightingBn
        BasicEffect_VSBasicPixelLightingTx BasicEffect_VSBasicPixelLightingTxBn BasicEffect_VSBasicPixelLightingTxVc
        BasicEffect_VSBasicPixelLightingTxVcBn BasicEffect_VSBasicPixelLightingVc BasicEffect_VSBasicPixelLightingVcBn
        BasicEffect_VSBasicTx BasicEffect_VSBasicTxNoFog BasicEffect_VSBasicTxVc BasicEffect_VSBasicTxVcNoFog
        BasicEffect_VSBasicVc BasicEffect_VSBasicVcNoFog BasicEffect_VSBasicVertexLighting
        BasicEffect_VSBasicVertexLightingBn BasicEffect_VSBasicVertexLightingTx BasicEffect_VSBasicVertexLightingTxBn
        BasicEffect_VSBasicVertexLightingTxVc BasicEffect_VSBasicVertexLightingTxVcBn BasicEffect_VSBasicVertexLightingVc
        BasicEffect_VSBasicVertexLightingVcBn
        # DGSLEffect
        DGSLEffect_main DGSLEffect_main1Bones DGSLEffect_main1BonesVc DGSLEffect_main2Bones
        DGSLEffect_main2BonesVc DGSLEffect_main4Bones DGSLEffect_main4BonesVc DGSLEffect_mainVc
        DGSLLambert_main DGSLLambert_mainTk DGSLLambert_mainTx DGSLLambert_mainTxTk
        DGSLPhong_main DGSLPhong_mainTk DGSLPhong_mainTx DGSLPhong_mainTxTk
        DGSLUnlit_main DGSLUnlit_mainTk DGSLUnlit_mainTx DGSLUnlit_mainTxTk
        # DebugEffect
        DebugEffect_PSHemiAmbient DebugEffect_PSRGBBiTangents DebugEffect_PSRGBNormals DebugEffect_PSRGBTangents
        DebugEffect_VSDebug DebugEffect_VSDebugBn DebugEffect_VSDebugBnInst DebugEffect_VSDebugInst
        DebugEffect_VSDebugVc DebugEffect_VSDebugVcBn DebugEffect_VSDebugVcBnInst DebugEffect_VSDebugVcInst
        # DualTextureEffect
        DualTextureEffect_PSDualTexture DualTextureEffect_PSDualTextureNoFog
        DualTextureEffect_VSDualTexture DualTextureEffect_VSDualTextureNoFog
        DualTextureEffect_VSDualTextureVc DualTextureEffect_VSDualTextureVcNoFog
        # EnvironmentMapEffect
        EnvironmentMapEffect_PSEnvMap EnvironmentMapEffect_PSEnvMapDualParabolaPixelLighting
        EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnel EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingFresnelNoFog
        EnvironmentMapEffect_PSEnvMapDualParabolaPixelLightingNoFog EnvironmentMapEffect_PSEnvMapNoFog
        EnvironmentMapEffect_PSEnvMapPixelLighting EnvironmentMapEffect_PSEnvMapPixelLightingFresnel
        EnvironmentMapEffect_PSEnvMapPixelLightingFresnelNoFog EnvironmentMapEffect_PSEnvMapPixelLightingNoFog
        EnvironmentMapEffect_PSEnvMapSpecular EnvironmentMapEffect_PSEnvMapSpecularNoFog
        EnvironmentMapEffect_PSEnvMapSpherePixelLighting EnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnel
        EnvironmentMapEffect_PSEnvMapSpherePixelLightingFresnelNoFog EnvironmentMapEffect_PSEnvMapSpherePixelLightingNoFog
        EnvironmentMapEffect_VSEnvMap EnvironmentMapEffect_VSEnvMapBn EnvironmentMapEffect_VSEnvMapFresnel
        EnvironmentMapEffect_VSEnvMapFresnelBn EnvironmentMapEffect_VSEnvMapOneLight EnvironmentMapEffect_VSEnvMapOneLightBn
        EnvironmentMapEffect_VSEnvMapOneLightFresnel EnvironmentMapEffect_VSEnvMapOneLightFresnelBn
        EnvironmentMapEffect_VSEnvMapPixelLighting EnvironmentMapEffect_VSEnvMapPixelLightingBn
        EnvironmentMapEffect_VSEnvMapPixelLightingBnSM4 EnvironmentMapEffect_VSEnvMapPixelLightingSM4
        # NormalMapEffect
        NormalMapEffect_PSNormalPixelLightingTx NormalMapEffect_PSNormalPixelLightingTxNoFog
        NormalMapEffect_PSNormalPixelLightingTxNoFogSpec NormalMapEffect_PSNormalPixelLightingTxNoSpec
        NormalMapEffect_VSNormalPixelLightingTx NormalMapEffect_VSNormalPixelLightingTxBn
        NormalMapEffect_VSNormalPixelLightingTxBnInst NormalMapEffect_VSNormalPixelLightingTxInst
        NormalMapEffect_VSNormalPixelLightingTxNoSpec NormalMapEffect_VSNormalPixelLightingTxNoSpecBn
        NormalMapEffect_VSNormalPixelLightingTxNoSpecBnInst NormalMapEffect_VSNormalPixelLightingTxNoSpecInst
        NormalMapEffect_VSNormalPixelLightingTxVc NormalMapEffect_VSNormalPixelLightingTxVcBn
        NormalMapEffect_VSNormalPixelLightingTxVcBnInst NormalMapEffect_VSNormalPixelLightingTxVcInst
        NormalMapEffect_VSNormalPixelLightingTxVcNoSpec NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBn
        NormalMapEffect_VSNormalPixelLightingTxVcNoSpecBnInst NormalMapEffect_VSNormalPixelLightingTxVcNoSpecInst
        NormalMapEffect_VSSkinnedPixelLightingTx NormalMapEffect_VSSkinnedPixelLightingTxBn
        NormalMapEffect_VSSkinnedPixelLightingTxNoSpec NormalMapEffect_VSSkinnedPixelLightingTxNoSpecBn
        # PBREffect
        PBREffect_PSConstant PBREffect_PSTextured PBREffect_PSTexturedEmissive
        PBREffect_PSTexturedEmissiveVelocity PBREffect_PSTexturedVelocity PBREffect_VSConstant
        PBREffect_VSConstantBn PBREffect_VSConstantBnInst PBREffect_VSConstantInst
        PBREffect_VSConstantVelocity PBREffect_VSConstantVelocityBn PBREffect_VSSkinned PBREffect_VSSkinnedBn
        # PostProcess
        PostProcess_PSBloomBlur PostProcess_PSBloomCombine PostProcess_PSBloomExtract PostProcess_PSCopy
        PostProcess_PSDownScale2x2 PostProcess_PSDownScale4x4 PostProcess_PSGaussianBlur5x5 PostProcess_PSMerge
        PostProcess_PSMonochrome PostProcess_PSSepia PostProcess_VSQuad PostProcess_VSQuadDual PostProcess_VSQuadNoCB
        # SkinnedEffect
        SkinnedEffect_PSSkinnedPixelLighting SkinnedEffect_PSSkinnedVertexLighting SkinnedEffect_PSSkinnedVertexLightingNoFog
        SkinnedEffect_VSSkinnedOneLightFourBones SkinnedEffect_VSSkinnedOneLightFourBonesBn
        SkinnedEffect_VSSkinnedOneLightOneBone SkinnedEffect_VSSkinnedOneLightOneBoneBn
        SkinnedEffect_VSSkinnedOneLightTwoBones SkinnedEffect_VSSkinnedOneLightTwoBonesBn
        SkinnedEffect_VSSkinnedPixelLightingFourBones SkinnedEffect_VSSkinnedPixelLightingFourBonesBn
        SkinnedEffect_VSSkinnedPixelLightingOneBone SkinnedEffect_VSSkinnedPixelLightingOneBoneBn
        SkinnedEffect_VSSkinnedPixelLightingTwoBones SkinnedEffect_VSSkinnedPixelLightingTwoBonesBn
        SkinnedEffect_VSSkinnedVertexLightingFourBones SkinnedEffect_VSSkinnedVertexLightingFourBonesBn
        SkinnedEffect_VSSkinnedVertexLightingOneBone SkinnedEffect_VSSkinnedVertexLightingOneBoneBn
        SkinnedEffect_VSSkinnedVertexLightingTwoBones SkinnedEffect_VSSkinnedVertexLightingTwoBonesBn
        # SpriteEffect
        SpriteEffect_SpritePixelShader SpriteEffect_SpritePixelShaderHeap
        SpriteEffect_SpriteVertexShader SpriteEffect_SpriteVertexShaderHeap
        # ToneMap
        ToneMap_PSACESFilmic ToneMap_PSACESFilmic_SRGB ToneMap_PSCopy ToneMap_PSHDR10
        ToneMap_PSReinhard ToneMap_PSReinhard_SRGB ToneMap_PSSaturate ToneMap_PSSaturate_SRGB
        ToneMap_PS_SRGB ToneMap_VSQuad
        # DX12 specific
        GenerateMips_main RootSig
    )
    
    foreach(shader ${SHADER_NAMES})
        if(NOT EXISTS "${SHADER_DIR}/${shader}.inc")
            file(WRITE "${SHADER_DIR}/${shader}.inc"
                "// Stub shader for cross-compilation - ${shader}\n"
                "// WARNING: Placeholder only. Replace with real shaders for runtime.\n"
                "const uint8_t ${shader}[] = { 0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00 };\n"
            )
        endif()
    endforeach()
    
    # TeapotData.inc
    if(NOT EXISTS "${SHADER_DIR}/TeapotData.inc")
        file(WRITE "${SHADER_DIR}/TeapotData.inc"
            "// Stub TeapotData for cross-compilation\n"
            "namespace { constexpr int TeapotPatchCount = 0; constexpr int TeapotVertexCount = 0; }\n"
        )
    endif()
endfunction()

# DirectXTK (DX11)
FetchContent_Declare(
        DirectXTK
        GIT_REPOSITORY "https://github.com/microsoft/DirectXTK.git"
        GIT_TAG "f5026eb34e7053b1aff325d38db107703f394974"
)

FetchContent_GetProperties(DirectXTK)
if(NOT directxtk_POPULATED)
    FetchContent_Populate(DirectXTK)
    
    set(COMPILED_SHADERS_DIR "${directxtk_SOURCE_DIR}/Src/Shaders/Compiled")
    file(MAKE_DIRECTORY "${COMPILED_SHADERS_DIR}")
    _generate_shader_stubs("${COMPILED_SHADERS_DIR}")
    
    message(WARNING "Cross-compiling DirectXTK with shader stubs. Effects won't render at runtime.")
    
    set(COMPILED_SHADERS "${COMPILED_SHADERS_DIR}" CACHE PATH "Compiled shader location" FORCE)
    set(USE_PREBUILT_SHADERS ON CACHE BOOL "Use prebuilt shaders" FORCE)
    
    add_subdirectory(${directxtk_SOURCE_DIR} ${directxtk_BINARY_DIR})
endif()

# DirectXTK12 (DX12)
FetchContent_Declare(
        DirectXTK12
        GIT_REPOSITORY "https://github.com/microsoft/DirectXTK12.git"
        GIT_TAG "528801aa6dd8d628c2f756c41a76d300f47de478"
)

FetchContent_GetProperties(DirectXTK12)
if(NOT directxtk12_POPULATED)
    FetchContent_Populate(DirectXTK12)
    
    set(COMPILED_SHADERS_DIR12 "${directxtk12_SOURCE_DIR}/Src/Shaders/Compiled")
    file(MAKE_DIRECTORY "${COMPILED_SHADERS_DIR12}")
    _generate_shader_stubs("${COMPILED_SHADERS_DIR12}")
    
    message(WARNING "Cross-compiling DirectXTK12 with shader stubs.")
    
    set(COMPILED_SHADERS "${COMPILED_SHADERS_DIR12}" CACHE PATH "Compiled shader location" FORCE)
    set(USE_PREBUILT_SHADERS ON CACHE BOOL "Use prebuilt shaders" FORCE)
    
    add_subdirectory(${directxtk12_SOURCE_DIR} ${directxtk12_BINARY_DIR})
endif()

# MSVC 19.35+ compatibility
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
    target_compile_options(DirectXTK PRIVATE /Zc:templateScope-)
    target_compile_options(DirectXTK12 PRIVATE /Zc:templateScope-)
endif()
