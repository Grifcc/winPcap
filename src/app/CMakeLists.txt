
add_executable(rev_win rev_win.cpp ${DET_SRC_DIR})
target_link_libraries(rev_win ${dynamic_libs} vps_pcap opencv_core)


add_executable(send_win send_win.cpp ${DET_SRC_DIR})
target_link_libraries(send_win ${dynamic_libs} ${OpenCV_LIBS})


message("Add project win_mode")
install(TARGETS rev_win DESTINATION ./)
install(TARGETS send_win DESTINATION ./)
