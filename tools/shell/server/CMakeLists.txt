add_executable(mshd main.cpp shell_server.cpp shell_session.cpp pid_watcher.cpp)
target_link_libraries(mshd shell sss routing krypto arsenal ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS mshd
    RUNTIME DESTINATION tools/shell)

#add_definitions(-DETCDIR="/usr/local/etc")
