include(FetchContent)

# Safetyhook
if(NOT POLYHOOK2_FUNCTION_HOOK)
    set(SAFETYHOOK_FETCH_ZYDIS ON)
endif()
message("Fetching safetyhook")

FetchContent_Declare(
        safetyhook
        GIT_REPOSITORY "https://github.com/cursey/safetyhook.git"
        GIT_TAG "bb8857590f59c132f02e66c5c4ef28158732f21c"
)
FetchContent_MakeAvailable(safetyhook)
set_property(TARGET safetyhook PROPERTY CXX_STANDARD 23)
if(NOT POLYHOOK2_FUNCTION_HOOK)
    set_property(TARGET safetyhook PROPERTY SAFETYHOOK_FETCH_ZYDIS ON)
endif()