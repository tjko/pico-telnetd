add_library(pico-telnetd-lib INTERFACE)
target_include_directories(pico-telnetd-lib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(pico-telnetd-lib INTERFACE pico_rand)
target_sources(pico-telnetd-lib INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/server.c
  ${CMAKE_CURRENT_LIST_DIR}/src/log.c
  ${CMAKE_CURRENT_LIST_DIR}/src/utils.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ringbuffer.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sha256crypt.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sha512crypt.c
  )

