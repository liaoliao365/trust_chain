project (optee_example_trust_chain C)

message(STATUS "CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_SYSROOT=${CMAKE_SYSROOT}")

set (SRC host/main.c)

add_executable (${PROJECT_NAME} ${SRC})

# set(CMAKE_SYSROOT "/home/lele/optee-qemu/buildroot/output/staging")

target_include_directories(${PROJECT_NAME}
			   PRIVATE ta/include
			   PRIVATE include)
			   
target_link_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SYSROOT}/usr/lib)

target_link_libraries (${PROJECT_NAME} PRIVATE teec jansson)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

