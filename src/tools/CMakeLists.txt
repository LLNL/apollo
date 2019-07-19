
include_directories(
    ../../include/external/nlohmann/
    )


add_executable(chkmodel chkmodel.cpp)
target_link_libraries(chkmodel apollo)

install(TARGETS
    chkmodel
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

