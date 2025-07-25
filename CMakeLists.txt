project (optee_example_trust_chain C)

set (SRC host/main.c)

set(CMAKE_SYSROOT "/home/lele/optee-qemu/buildroot/output/host/i586-buildroot-linux-gnu/sysroot")
set(OPTEE_OS_DIR "/home/lele/optee-qemu/optee_os")
set(TEEC_EXPORT "/home/lele/optee-qemu/optee_os/out/arm/export-ta_arm32")

add_executable (${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
			   ta/include
			   include
			   ${CMAKE_SYSROOT}/usr/include
			   ${OPTEE_OS_DIR}/lib/libutee/include
			   ${TEEC_EXPORT}/include)

target_link_libraries (${PROJECT_NAME} PRIVATE teec jansson)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
