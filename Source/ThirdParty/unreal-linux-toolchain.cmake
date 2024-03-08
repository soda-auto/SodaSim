set (UE_LIBCXX ~/UnrealEngine-5.3.2-release/Engine/Source/ThirdParty/Unix/LibCxx)
set (LINUX_MULTIARCH_ROOT ~/v22_clang-16.0.6-centos7)
set (TARGET_TRIPLE x86_64-unknown-linux-gnu)
set (LD_FLAGS "-nodefaultlibs -stdlib=libc++ -lc -lm -lgcc_s -lgcc ${UE_LIBCXX}/lib/Unix/x86_64-unknown-linux-gnu/libc++.a ${UE_LIBCXX}/lib/Unix/x86_64-unknown-linux-gnu/libc++abi.a")

set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_CROSSCOMPILING TRUE)
set (CMAKE_SYSTEM_PROCESSOR x86_64)
set (CMAKE_SYSROOT ${LINUX_MULTIARCH_ROOT}/${TARGET_TRIPLE})
set (CMAKE_C_COMPILER  ${CMAKE_SYSROOT}/bin/clang)
set (CMAKE_CXX_COMPILER ${CMAKE_SYSROOT}/bin/clang++)
set (CMAKE_C_COMPILER_TARGET ${TARGET_TRIPLE})
set (CMAKE_CXX_COMPILER_TARGET ${TARGET_TRIPLE})
set (CMAKE_ASM_COMPILER_TARGET ${TARGET_TRIPLE})
set (CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT} )
set (CMAKE_CXX_FLAGS "-nostdinc++ -nostdlib++ -fPIC -I${UE_LIBCXX}/include -I${UE_LIBCXX}/include/c++/v1 ")
set (CMAKE_MODULE_LINKER_FLAGS ${LD_FLAGS})
set (CMAKE_EXE_LINKER_FLAGS_INIT ${LD_FLAGS})
set (CMAKE_SHARED_LINKER_FLAGS_INIT ${LD_FLAGS})