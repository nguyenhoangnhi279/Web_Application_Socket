# Remote Desktop Control System (Web-based)

![C++](https://img.shields.io/badge/Language-C++17-blue.svg) ![Frontend](https://img.shields.io/badge/Frontend-HTML5%20%2F%20JS-orange.svg) ![Protocol](https://img.shields.io/badge/Protocol-WebSocket-green.svg) ![Platform](https://img.shields.io/badge/Platform-Windows-0078D6.svg)

> **ONEFIVE**
>
> Hệ thống điều khiển và giám sát máy tính từ xa thông qua giao diện Web, sử dụng giao thức WebSocket hoạt động trên mạng LAN/VPN.

## Giới thiệu

Dự án này là một giải pháp **Remote Desktop** theo mô hình Client-Server. Client của hệ thống chạy hoàn toàn trên trình duyệt Web, giúp người quản trị có thể điều khiển máy tính mục tiêu từ bất kỳ thiết bị nào (PC, Mobile, Tablet) mà không cần cài đặt phần mềm.

## Tính năng chính

Hệ thống cung cấp bộ công cụ toàn diện để quản trị hệ thống Windows từ xa:

| Module | Chức năng chi tiết |
| :--- | :--- |
| **Screenshot** | Chụp màn hình Desktop thời gian thực. |
| **Keylogger** | Giám sát bàn phím, ghi lại phím bấm (hỗ trợ phím chức năng). |
| **Webcam** | Quay video hoặc Stream hình ảnh từ Webcam (sử dụng FFmpeg). |
| **Power Control** | Tắt máy, Khởi động lại, Đăng xuất. |
| **TaskManager** | Liệt kê, tìm kiếm ứng dụng/tiến trình. Xem RAM usage. Kill Process. |
| **File Transfer** | Duyệt file, Upload và Download file tốc độ cao. |
| **Net Monitor** | Giám sát các kết nối mạng TCP/IP đang mở trên máy. |

## Công nghệ sử dụng

### Backend (Server - Máy bị điều khiển)
* **Ngôn ngữ:** C++ 17 (Visual Studio).
* **Core:** Windows API (Winsock2, PSAPI, GDI+, User32).
* **Media:** FFmpeg (Xử lý Video/Webcam), DirectShow.
* **Data:** JSON (nlohmann/json), Base64 Encoding.
* **Multithreading:** Xử lý đa luồng cho Socket và Keylogger.

### Frontend (Client - Máy quản trị)
* **Giao diện:** HTML5, CSS3 (Bootstrap 5 - Dark Mode).
* **Logic:** JavaScript (Native WebSocket API).
* **Hiệu ứng:** Particles.js.

## Yêu cầu cài đặt

1.  **Hệ điều hành:** Windows 10/11 (Server).
2.  **Môi trường Build:** Visual Studio 2019/2022 (C++ Desktop Development).
3.  **Công cụ phụ trợ:**
    * **FFmpeg:** Cần có file `ffmpeg.exe` để chạy tính năng Webcam.

## 📥 Hướng dẫn chạy (Installation & Usage)

### Bước 1: Build Server (C++)
1.  Mở Solution bằng Visual Studio.
2.  Chuyển chế độ Build sang **Release** (x64 hoặc x86).
3.  Build Project (`Ctrl + Shift + B`).
4.  File thực thi `RemoteServer.exe` sẽ nằm trong thư mục `x64/Release`.

### Bước 2: Cấu hình FFmpeg
1.  Tải `ffmpeg.exe` (bản static build).
2.  Copy file `ffmpeg.exe` vào cùng thư mục với `RemoteServer.exe` (hoặc thư mục `Tools/` tùy theo code quy định).

### Bước 3: Chạy Server
1.  Click chuột phải vào `RemoteServer.exe` -> Chọn **Run as Administrator**.
    * *Bắt buộc phải chạy quyền Admin để dùng tính năng Shutdown/Kill Process.*
2.  Console hiện thông báo:
    ```
    >> Server dang chay tai PORT: 8080
    >> Waiting for Web Client...
    ```

### Bước 4: Kết nối từ Client
1.  Mở file `index.html` bằng trình duyệt (Chrome/Edge).
2.  Nhập địa chỉ IP của máy Server (ví dụ: `192.168.1.10:8080`).
    * *Nếu chạy cùng máy thì nhập `localhost:8080`.*
3.  Nhấn nút **Connect**.

## ⚠️ Lưu ý quan trọng

* **Tường lửa (Firewall):** Nếu kết nối từ máy khác trong mạng LAN, hãy đảm bảo Windows Firewall đã **cho phép Port 8080**  hoặc tắt tạm thời Firewall.
* **Antivirus:** Một số phần mềm diệt virus có thể nhận diện `RemoteServer.exe` là mã độc do hành vi Keylogger. Hãy thêm vào danh sách loại trừ khi chạy thử nghiệm.
* **Lag/Delay:** Khi truyền tải file lớn hoặc xem Webcam, độ trễ phụ thuộc vào tốc độ mạng LAN của bạn.

## Tác giả
**Nhóm 15 - Lớp 24TNT1 - HCMUS**
* Nguyễn Hoàng Ý Nhi
* Đỗ Lê Phong Phú
* Đỗ Bá Danh Lộc

---
*Designed with ❤️ by OneFive Team.*
