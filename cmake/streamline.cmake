include(FetchContent)

# Streamline - headers only
FetchContent_Declare(
        streamline
        GIT_REPOSITORY "https://github.com/NVIDIAGameWorks/Streamline.git"
        GIT_TAG "${STREAMLINE_VERSION}"
)
message("Fetching streamline ${STREAMLINE_VERSION}")
FetchContent_Populate(streamline)

