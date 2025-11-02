include(FetchContent)

# Streamline
FetchContent_Declare(
        streamline
        GIT_REPOSITORY "https://github.com/NVIDIAGameWorks/Streamline.git"
        GIT_TAG "${STREAMLINE_VERSION}"
)
message("Fetching streamline ${STREAMLINE_VERSION}")
FetchContent_MakeAvailable(streamline)
#set_property(TARGET streamline PROPERTY CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#set_property(TARGET streamline PROPERTY STREAMLINE_IMPORT_AS_INTERFACE ON)

