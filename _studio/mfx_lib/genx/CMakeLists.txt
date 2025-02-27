# Copyright (c) 2018-2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

if(MFX_ENABLE_KERNELS)
  mfx_include_dirs( )

  set( MSDK_GENX_ROOT ${MSDK_LIB_ROOT}/genx )

  if (BUILD_KERNELS)
    # Here we define build steps to generate c-array binary kernels. If you
    # don't use BUILD_KERNELS options these kernels will just be used from
    # pre-built form. If you will regenerate kernels you may notice the
    # difference from the per-built kernels in git-diff.

    # Helper application to generate c-array data representation of .isa binary
    # kernels.
    add_executable(embed_isa ${MSDK_GENX_ROOT}/tools/embed_isa.c)
    set_target_properties(embed_isa PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    # Generates CM kernel suitable for usage in C/C++ application sources
    #   ${src} - input CM kernel source file
    #   ${genx} - platform generation level
    #   ${out_dir} - directory to generate output
    # Generated outputs for ${src}=path/file.cpp, ${genx}=gen9:
    #   ${out_dir}/file_gen9_isa.h
    #   ${out_dir}/file_gen9_isa.cpp
    function(add_cmc_target src genx out_dir)
      get_filename_component(tgt ${src} NAME_WE)
      add_custom_command(
        OUTPUT ${tgt}_${genx}.isa
        DEPENDS embed_isa ${src}
        COMMAND ${CMC}
          -c -Qxcm -Qxcm_release -Qxcm_print_asm_count
          -menableiga -mCM_init_global -mCM_printregusage
          -Dtarget_${genx} -Qxcm_jit_target=${genx}
          ${src} -o ${tgt}_${genx}.isa)

      add_custom_command(
        OUTPUT ${tgt}_${genx}_isa.cpp ${tgt}_${genx}_isa.h
        DEPENDS embed_isa ${tgt}_${genx}.isa
        COMMAND embed_isa ${tgt}_${genx}.isa)
      add_custom_command(
        OUTPUT ${out_dir}/${tgt}_${genx}_isa.h ${out_dir}/${tgt}_${genx}_isa.cpp
        DEPENDS ${tgt}_${genx}_isa.cpp ${tgt}_${genx}_isa.h
        COMMAND ${CMAKE_COMMAND} -E copy ${tgt}_${genx}_isa.h ${out_dir}
        COMMAND ${CMAKE_COMMAND} -E copy ${tgt}_${genx}_isa.cpp ${out_dir})
    endfunction()
  endif()

  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/asc/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/copy_kernels/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/field_copy/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/h264_encode/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/mctf/isa )
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../cmrt_cross_platform/include )

  set( defs "" )
  set( sources "" )
  set( sources.plus "" )

  if( MFX_ENABLE_ASC )
    if(BUILD_KERNELS)
      foreach(platform gen12lp)
        add_cmc_target(${MSDK_GENX_ROOT}/asc/src/genx_scd.cpp ${platform} ${MSDK_GENX_ROOT}/asc/isa)
      endforeach()
    endif()

    set( prefix ${MSDK_GENX_ROOT}/asc/isa )
    list( APPEND sources
        ${prefix}/genx_scd_gen12lp_isa.cpp
    )
  endif()

  if(BUILD_KERNELS)
    foreach(platform gen12lp)
      add_cmc_target(${MSDK_GENX_ROOT}/copy_kernels/src/genx_copy_kernel.cpp ${platform} ${MSDK_GENX_ROOT}/copy_kernels/isa)
    endforeach()
  endif()

  set( prefix ${MSDK_GENX_ROOT}/copy_kernels/isa )
  list( APPEND sources
    ${prefix}/genx_copy_kernel_gen12lp_isa.cpp
  )

  if(BUILD_KERNELS)
    foreach(platform gen12lp)
      add_cmc_target(${MSDK_GENX_ROOT}/field_copy/src/genx_fcopy.cpp ${platform} ${MSDK_GENX_ROOT}/field_copy/isa)
    endforeach()
  endif()

  set( prefix ${MSDK_GENX_ROOT}/field_copy/isa )
  list( APPEND sources
    ${prefix}/genx_fcopy_gen12lp_isa.cpp
  )

  if(BUILD_KERNELS)
    foreach(platform gen12lp)
      add_cmc_target(${MSDK_GENX_ROOT}/h264_encode/src/genx_histogram.cpp ${platform} ${MSDK_GENX_ROOT}/h264_encode/isa)
      add_cmc_target(${MSDK_GENX_ROOT}/h264_encode/src/genx_simple_me.cpp ${platform} ${MSDK_GENX_ROOT}/h264_encode/isa)
    endforeach()
  endif()

  set( prefix ${MSDK_GENX_ROOT}/h264_encode/isa )
  list( APPEND sources
    ${MSDK_GENX_ROOT}/h264_encode/src/genx_simple_me_proto.cpp
    ${prefix}/genx_simple_me_gen12lp_isa.cpp
    ${prefix}/genx_histogram_gen12lp_isa.cpp
  )

  if(MFX_ENABLE_MCTF)
    if(BUILD_KERNELS)
      foreach(platform gen12lp)
        add_cmc_target(${MSDK_GENX_ROOT}/mctf/src/genx_me.cpp ${platform} ${MSDK_GENX_ROOT}/mctf/isa)
        add_cmc_target(${MSDK_GENX_ROOT}/mctf/src/genx_mc.cpp ${platform} ${MSDK_GENX_ROOT}/mctf/isa)
        add_cmc_target(${MSDK_GENX_ROOT}/mctf/src/genx_sd.cpp ${platform} ${MSDK_GENX_ROOT}/mctf/isa)
      endforeach()
    endif()

    set( prefix ${MSDK_GENX_ROOT}/mctf/isa )
    list( APPEND sources
      ${prefix}/genx_mc_gen12lp_isa.cpp
      ${prefix}/genx_me_gen12lp_isa.cpp
      ${prefix}/genx_sd_gen12lp_isa.cpp
    )
  endif()

  add_library(genx STATIC ${sources})
  set_property(TARGET genx PROPERTY FOLDER "kernels")

  target_include_directories(genx
    PUBLIC
      asc/isa
      copy_kernels/isa
      field_copy/isa
      h264_encode/isa
      mctf/isa
    )

  target_link_libraries(genx
    PUBLIC
      mfx_static_lib
    PRIVATE
      mfx_sdl_properties
    )
endif()

include(sources_ext.cmake OPTIONAL)