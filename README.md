# 🚗 Automotive Emergency Call & Rollover Detection System (eCall)

Hệ thống nhúng giám sát góc nghiêng, va chạm xe và tự động gửi tin nhắn cảnh báo khẩn cấp (SMS khẩn cấp kèm tọa độ GPS thời gian thực) thông qua mạng 4G LTE. Dự án được ứng dụng mô hình các hệ thống eCall trên ô tô hiện đại.

## 🎯 Tính năng nổi bật
- **Phát hiện lật xe thời gian thực:** Đọc và tính toán góc nghiêng ($Roll$) từ cảm biến gia tốc/quay 6 trục MPU-6050 thông qua giao thức I2C.
- **Phát hiện va chạm:** Giả lập và nhận diện va chạm lập tức, ưu tiên xử lý ngắt và gửi tín hiệu khẩn cấp.
- **Định vị chính xác:** Thu thập dữ liệu NMEA (`$GPGGA`) từ module GPS qua giao thức UART, phân tích thuật toán chuyển đổi sang tọa độ thập phân (Latitude/Longitude).
- **Cảnh báo từ xa qua 4G LTE:** Giao tiếp tập lệnh AT với module SIMCom A7682S-V1 (4G Cat 1) để gửi tin nhắn SMS cảnh báo tai nạn trực tiếp đến điện thoại người dùng.
- **Cảnh báo tại chỗ:** Hệ thống còi hú (Buzzer) và đèn LED nhấp nháy liên tục tại hiện trường vụ tai nạn.

## 🛠 Sơ đồ phần cứng & Kết nối pinout (Arduino Uno/Nano)

| Linh kiện / Module | Chân Arduino | Giao thức / Chức năng | Ghi chú |
| :--- | :--- | :--- | :--- |
| **MPU-6050** | `SDA` / `SCL` | I2C (Address `0x68`) | Đo góc nghiêng xe |
| **GPS Module** | `D8 (TX)` / `D9 (RX)`| UART (`SoftwareSerial` 9600 baud) | Đọc dữ liệu định vị |
| **SIMCom A7682S-V1** | `D10 (RX)` / `D11 (TX)`| UART (`SoftwareSerial` 9600 baud) | Module 4G gửi SMS |
| **Push Button** | `D2` | Digital Input (`INPUT_PULLUP`) | Giả lập va chạm |
| **LED Cảnh báo** | `D3` | Digital Output | Nháy đèn báo động |
| **Buzzer** | `D7` | Digital Output | Còi hú báo động |

> **⚠️ Lưu ý quan trọng về phần cứng:**
> - Module 4G A7682S-V1 cần dòng đỉnh (Peak Current) lên tới **2A**. Cần cấp nguồn riêng 5V/2A cho module và **bắt buộc nối chung Mass (GND)** với bo mạch Arduino.
> - Cần cấu hình Baud rate của module A7682S về `9600` bằng lệnh `AT+IPR=9600` 

## 💻 Cấu trúc mã nguồn
- `RDS.ino`: Chương trình chính (Main firmware), quản lý luồng thực thi liên tục đọc cảm biến, xử lý chuỗi NMEA GPS và điều khiển giao tiếp GSM.

## 🚀 Hướng dẫn cài đặt & Nạp code
1. Tải và cài đặt [Arduino IDE](https://www.arduino.cc/en/software).
2. Kết nối phần cứng theo đúng bảng Pinout ở trên.
3. Mở file `dacn3.ino`. Thay đổi số điện thoại nhận cảnh báo tại dòng:
   ```cpp
   const char *phoneNumber = "+84xxxxxxxxx";