file(GLOB_RECURSE test_srcs *.cpp)
set(test_bin ${PROJECT_NAME}-unittests)
add_executable(${test_bin} ${test_srcs})

foreach(fpath ${test_srcs})
    get_filename_component(tname ${fpath} NAME_WE)
    if(${tname} MATCHES "^test_")
        string(SUBSTRING ${tname} 5 -1 tdisplay)
        add_test(NAME ${tdisplay} COMMAND ${test_bin} -\# [\#${tname}])
    endif()
endforeach()

target_link_libraries(${test_bin} eventbus)
set_target_properties(${test_bin} PROPERTIES CXX_STANDARD 11)
target_include_directories(${test_bin} SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/thirdparty/include")

add_compile_options(-Wall -Wpedantic -Wextra -Werror)
