#include <SoftwareSerial.h>
#include <Wire.h>
// #include <TinyGPS++.h> // Đã bỏ thư viện TinyGPS++

int Rx_pin = 9; // Nối với chân TX của module GPS
int Tx_pin = 8; // Nối với chân RX của module GPS (nếu bạn muốn gửi lệnh đến GPS)

SoftwareSerial SerialGPS(Rx_pin, Tx_pin);
SoftwareSerial GSM(10, 11);

// TinyGPSPlus gps; // Đã bỏ đối tượng TinyGPSPlus

#define BUZ 7
#define LED 3
int button = 2;
double roll; // Biến roll từ MPU6050

#include <math.h> // Để dùng hàm atan2 và M_PI

const int GSM_POWER_ON_DELAY = 20000; // 20 giây đợi module GSM khởi động
const char *phoneNumber = "+84812835572"; // Số điện thoại nhận tin nhắn
const int MPU_ADDR = 0x68;  // Địa chỉ I2C của MPU-6050

// Các biến toàn cục để lưu trữ dữ liệu GPS đã phân tích
float currentLatitude = 0.0;
float currentLongitude = 0.0;
bool gpsFix = false; // Biến cờ để kiểm tra xem có fix GPS chưa

// Khai báo nguyên mẫu hàm (prototype) vì chúng được gọi trước khi định nghĩa
void readMPU6050();
void buzled();
void SendMessage();
void processGpsString(String nmeaString);
float convertNMEAtoDecimal(String nmeaCoord);


void setup() {
  pinMode(button, INPUT_PULLUP); // Dùng INPUT_PULLUP cho nút bấm nếu nối với GND

  Serial.begin(115200); // Serial Monitor để debug
  SerialGPS.begin(9600); // Baud rate của GPS (thường là 9600)
  GSM.begin(9600);   // Baud rate của module GSM (A7682A thường dùng 115200 hoặc 9600)

  pinMode(BUZ, OUTPUT);
  pinMode(LED, OUTPUT); // Khai báo chân LED là OUTPUT

  // Khởi động MPU6050
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);   // Thanh ghi PWR_MGMT_1
  Wire.write(0);      // Tắt chế độ ngủ (wake up MPU6050)
  Wire.endTransmission(true);

  Serial.println("Waiting for GSM module to boot...");
  delay(GSM_POWER_ON_DELAY); // Chờ GSM khởi động
  Serial.println("GSM module ready.");
}

void loop(){
  // ĐỌC VÀ XỬ LÝ DỮ LIỆU GPS LIÊN TỤC TRONG LOOP CHÍNH
  static String gpsBuffer = ""; 
  while (SerialGPS.available()) {
    char c = SerialGPS.read();
    gpsBuffer += c;
    if (c == '\n') { // Nếu nhận được ký tự xuống dòng, có thể là một chuỗi NMEA hoàn chỉnh
      processGpsString(gpsBuffer);
      gpsBuffer = ""; // Xóa bộ đệm sau khi xử lý
    }
  }
  // --- KẾT THÚC XỬ LÝ GPS LIÊN TỤC ---

  // Kiểm tra nút bấm để phát hiện va chạm giả lập
  readMPU6050();
  if (digitalRead(button) == LOW) { // Nếu nút bấm được nhấn (nối GND)
    Serial.println("Collision Detected by Button");
    Serial.println(roll);
    delay(300); // Độ trễ nhỏ để tránh đọc lỗi nút
    roll = 900; // Gán giá trị đặc biệt cho va chạm
    SendMessage(); // Gửi tin nhắn
    buzled(); // Báo động còi và đèn
    // Nếu bạn muốn dừng chương trình sau khi phát hiện và gửi tin, hãy cân nhắc hàm exit()
    
  }
 // Đọc dữ liệu từ MPU6050 để tính roll

  // Kiểm tra góc roll để phát hiện lật xe
  // Các ngưỡng 180 và 240 cần được hiệu chỉnh thực tế với xe của bạn
  if (((roll < 180) || (roll > 240)) && (roll != 900)) { // Nếu lật và không phải là va chạm giả lập
    Serial.print("Car Flipped! Roll angle: ");
    Serial.println(roll);
    SendMessage(); // Gửi tin nhắn
    buzled(); // Báo động còi và đèn
  }

  // In trạng thái roll (chỉ để debug)
  Serial.print("Car is not Flipped. Current roll: ");
  Serial.println(roll);

  delay(100); // Độ trễ nhỏ giữa các lần kiểm tra
}

// Hàm đọc MPU6050 và tính góc roll
void readMPU6050() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);   // Địa chỉ thanh ghi ACCEL_XOUT_H (bắt đầu từ X, cần 6 bytes cho X,Y,Z)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);   // Đọc 6 byte (AccelX_H, AccelX_L, AccelY_H, AccelY_L, AccelZ_H, AccelZ_L)

  // Đọc gia tốc X, Y, Z
  int16_t ax = (Wire.read() << 8 | Wire.read()); // Đọc AccelX_H và AccelX_L
  int16_t ay = (Wire.read() << 8 | Wire.read()); // Đọc AccelY_H và AccelY_L
  int16_t az = (Wire.read() << 8 | Wire.read()); // Đọc AccelZ_H và AccelZ_L

  // Chuyển đổi sang đơn vị g (mặc định ±2g, 16384 LSB/g)
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  // Tính góc roll (đơn vị độ) từ gia tốc Y và Z
  // atan2(y, x) trả về góc trong khoảng -PI đến PI (-180 đến 180 độ)
  roll = atan2(ay_g, az_g) * 180.0 / M_PI;
  roll = (roll < 0) ? roll + 360 : roll; // Chuyển về khoảng 0-360°
}

// Hàm báo động còi và đèn LED
void buzled(){
  unsigned long buzzStartTime = millis();
  while (millis() - buzzStartTime < 10000) { // Báo động trong 10 giây
    digitalWrite(LED, HIGH);
    digitalWrite(BUZ, HIGH);
    delay(500); // Nháy nhanh hơn
    digitalWrite(BUZ, LOW);
    digitalWrite(LED, LOW);
    delay(500); // Nháy nhanh hơn
  }
}

// Hàm gửi tin nhắn SMS
void SendMessage(){
  Serial.println("Attempting to send SMS...");
  GSM.println("AT+CMGF=1");     // Đặt GSM Module vào chế độ Text Mode
  delay(1000); // Đợi phản hồi OK từ module

  GSM.print("AT+CMGS=\"");
  GSM.print(phoneNumber);
  GSM.println("\""); // Kết thúc lệnh CMGS
  delay(1000); // Đợi module sẵn sàng nhận nội dung tin nhắn (thường là dấu '>')

  // Nội dung tin nhắn
  GSM.println("!!!! Car Incident Detected !!!!");

  // Thêm thông tin GPS nếu có
  if (gpsFix) { // Kiểm tra xem dữ liệu GPS có hợp lệ không
    GSM.print("Location: Lat ");
    GSM.print(currentLatitude, 6); // Vĩ độ với 6 chữ số thập phân
    GSM.print(", Lon ");
    GSM.println(currentLongitude, 6); // Kinh độ với 6 chữ số thập phân
    Serial.print("GPS: Lat ");
    Serial.print(currentLatitude, 6);
    Serial.print(", Lon ");
    Serial.println(currentLongitude, 6);
  } else {
    GSM.println("Location: GPS not available (No Fix)");
    Serial.println("GPS: No Fix available.");
  }

  // Thêm thông tin loại sự cố
  if (roll == 900) {
    GSM.println("Type: The Car is Collided");
    Serial.println("Type: The Car is Collided");
  } else if (((roll < 180) || (roll > 240))) { // Chỉ cần kiểm tra nếu không phải 900
    GSM.println("Type: EMERGENCY: The Car is flipped");
    Serial.println("Type: EMERGENCY: The Car is flipped");
  } else {
    GSM.println("Type: Incident detected but not flipped/collided (Unusual roll value)");
  }

  GSM.write((char)26); // Gửi ký tự CTRL+Z (ASCII 26) để kết thúc tin nhắn
  Serial.println("Sent CTRL+Z. Waiting for SMS confirmation...");

  // Đợi GSM gửi tin nhắn và phản hồi
  unsigned long smsTimeout = millis() + 20000; // Chờ 20 giây cho phản hồi SMS
  while (millis() < smsTimeout) {
    if (GSM.available()) {
      Serial.write(GSM.read()); // In phản hồi của GSM ra Serial Monitor
    }
  }
  Serial.println("\nSMS send attempt finished.");
}

// Hàm để xử lý chuỗi NMEA và trích xuất dữ liệu GPS
void processGpsString(String nmeaString) {
  // Chỉ quan tâm đến chuỗi GPGGA cho tọa độ và chất lượng fix
  if (nmeaString.startsWith("$GPGGA")) {
    int commaIndex[20]; // Array để lưu vị trí các dấu phẩy 
    int count = 0;
    for (int i = 0; i < nmeaString.length() && count < 20; i++) { 
      if (nmeaString.charAt(i) == ',') {
        commaIndex[count] = i;
        count++;
      }
    }

    // Đảm bảo có đủ trường cần thiết cho Lat/Lon/Quality (cần ít nhất 6 dấu phẩy đầu tiên)
    if (count >= 6) { // Index của chất lượng là 6 (sau dấu phẩy thứ 6), nên cần ít nhất 6 dấu phẩy
      // 1. Lấy trạng thái chất lượng GPS (field 6, sau dấu phẩy thứ 5, index của dấu phẩy là 5)
      String qualityStr = nmeaString.substring(commaIndex[5] + 1, commaIndex[6]);
      int quality = qualityStr.toInt();

      if (quality >= 1) { // 
        gpsFix = true;

        // 2. Lấy vĩ độ (field 2, sau dấu phẩy thứ 1)
        String latStr = nmeaString.substring(commaIndex[1] + 1, commaIndex[2]);
        String latDir = nmeaString.substring(commaIndex[2] + 1, commaIndex[3]); // Hướng Bắc/Nam

        // Chuyển đổi định dạng ddmm.mmmm thành dd.dddddd
        currentLatitude = convertNMEAtoDecimal(latStr);
        if (latDir == "S") {
          currentLatitude *= -1; // Nếu là Nam bán cầu, đổi dấu
        }

        // 3. Lấy kinh độ (field 4, sau dấu phẩy thứ 3)
        String lonStr = nmeaString.substring(commaIndex[3] + 1, commaIndex[4]);
        String lonDir = nmeaString.substring(commaIndex[4] + 1, commaIndex[5]); // Hướng Đông/Tây

        // Chuyển đổi định dạng dddmm.mmmm thành ddd.dddddd
        currentLongitude = convertNMEAtoDecimal(lonStr);
        if (lonDir == "W") {
          currentLongitude *= -1; // Nếu là Tây bán cầu, đổi dấu
        }
      } else {
        gpsFix = false; // Không có fix GPS
      }
    } else {
      gpsFix = false; // Chuỗi GPGGA không đúng định dạng
    }
  }
}

// Hàm chuyển đổi định dạng NMEA (ddmm.mmmm hoặc dddmm.mmmm) sang thập phân
float convertNMEAtoDecimal(String nmeaCoord) {
  float degrees;
  float minutes;

  // Tìm vị trí của dấu chấm thập phân
  int dotIndex = nmeaCoord.indexOf('.');

  // Lấy phần phút và giây (từ dotIndex - 2 đến hết)
  // Ví dụ: "1559.57881" -> minutes = "59.57881"
  // Ví dụ: "10813.93184" -> minutes = "13.93184"
  if (dotIndex != -1 && nmeaCoord.length() >= dotIndex + 1 && dotIndex >= 2) {
    minutes = nmeaCoord.substring(dotIndex - 2).toFloat();
    // Lấy phần độ
    degrees = nmeaCoord.substring(0, dotIndex - 2).toFloat();
  } else {
    return 0.0; // Trả về 0 nếu định dạng không đúng
  }

  return degrees + (minutes / 60.0);
}