cmake_minimum_required(VERSION 3.15)

project(RemoteControlServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(WIN32_LEAN_AND_MEAN _HAS_STD_BYTE=0)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Features
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty
)

set(SOURCES
    main.cpp
    
    # Module Mạng
    Network/ServerNetwork.cpp
    
    # Module Xử lý trung tâm
    Core/CommandHandler.cpp
 
)

add_executable(RemoteServer ${SOURCES})

target_link_libraries(RemoteServer PRIVATE
    ws2_32      # Winsock (Kết nối mạng)
    gdiplus     # GDI+ (Chụp màn hình)
    crypt32     # CryptoAPI (Mã hóa Handshake WebSocket)
    psapi       # Process Status API (Lấy thông tin RAM)
    shell32     # Shell API (Dùng lệnh Start App/Process)
    user32      # User32 (Các hàm cửa sổ, bàn phím...)
    kernel32    # Kernel32 (Hàm hệ thống cơ bản)
    gdi32       # GDI (Hỗ trợ đồ họa)
)

# (Tùy chọn) Lệnh copy file ffmpeg.exe vào thư mục build sau khi biên dịch xong
# Giúp bạn đỡ phải copy tay file ffmpeg mỗi lần build lại
# add_custom_command(TARGET RemoteServer POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     "${CMAKE_SOURCE_DIR}/../../Tools/ffmpeg.exe"
#     $<TARGET_FILE_DIR:RemoteServer>
# )