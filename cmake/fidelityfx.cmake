include(FetchContent)

# FidelityFX SDK - headers only (for FSR 2/3 struct layouts)
FetchContent_Declare(
        fidelityfx
        GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK.git"
        GIT_TAG "v1.1.2"
        GIT_SHALLOW TRUE
)
message("Fetching FidelityFX SDK (headers only)")
FetchContent_Populate(fidelityfx)

# We only need the headers for struct layouts, not the actual SDK binaries
# Headers are located in sdk/include/
set(FIDELITYFX_INCLUDE_DIR ${fidelityfx_SOURCE_DIR}/sdk/include)
