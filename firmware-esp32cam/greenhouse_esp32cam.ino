/*******************************************************************************
 * GREENHOUSE ESP32-CAM FIRMWARE
 * ============================================================================
 * Chức năng:
 * - HTTP WebServer on port 80
 * - Endpoints:
 *   - /stream: MJPEG stream
 *   - /capture: Single JPEG image
 *   - /led?duty=0..255: Control flash LED (GPIO4 PWM)
 *   - /control: Set camera parameters (framesize, quality, hmirror, vflip)
 *   - /status: Get system status (JSON)
 * - PSRAM enabled if available
 * - SoftAP fallback if STA fails
 *
 * Hardware: AI-Thinker ESP32-CAM
 * Flash LED: GPIO4 (PWM channel 0)
 ******************************************************************************/

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include <WiFi.h>
#include <Preferences.h>

// ==================== CAMERA PINS (AI-Thinker) ====================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN     4

// ==================== CONFIGURATION ====================
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASS = "YourWiFiPassword";

const char* AP_SSID = "ESP32-CAM-GH";
const char* AP_PASS = "12345678";

// ==================== GLOBALS ====================
httpd_handle_t camera_httpd = NULL;

Preferences prefs;

// Camera config
framesize_t current_framesize = FRAMESIZE_SVGA;  // 800x600
int current_quality = 12;  // 10-63, lower = better
bool current_hmirror = false;
bool current_vflip = false;

unsigned long lastFrameTime = 0;
unsigned long frameCount = 0;
float fps = 0.0;

// ==================== SETUP ====================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable brownout detector

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("ESP32-CAM Greenhouse starting...");

  // Init flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  // Init preferences
  prefs.begin("esp32cam", false);
  current_framesize = (framesize_t)prefs.getInt("framesize", FRAMESIZE_SVGA);
  current_quality = prefs.getInt("quality", 12);
  current_hmirror = prefs.getBool("hmirror", false);
  current_vflip = prefs.getBool("vflip", false);

  // Camera config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Use PSRAM if available
  if (psramFound()) {
    Serial.println("PSRAM found, using UXGA");
    config.frame_size = current_framesize;
    config.jpeg_quality = current_quality;
    config.fb_count = 2;
  } else {
    Serial.println("No PSRAM, using SVGA");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }

  // Set camera settings
  sensor_t* s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_framesize(s, current_framesize);
    s->set_quality(s, current_quality);
    s->set_hmirror(s, current_hmirror ? 1 : 0);
    s->set_vflip(s, current_vflip ? 1 : 0);
  }

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("Camera Stream: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/stream");
  } else {
    Serial.println("\nWiFi failed, starting AP");
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  // Start web server
  startCameraServer();

  Serial.println("Setup complete!");
}

// ==================== MAIN LOOP ====================
void loop() {
  delay(100);
  // Server runs in background
}

// ==================== WEB SERVER ====================
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  // Stream handler
  httpd_uri_t stream_uri = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
  };

  // Capture handler
  httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_handler,
    .user_ctx = NULL
  };

  // LED control handler
  httpd_uri_t led_uri = {
    .uri = "/led",
    .method = HTTP_GET,
    .handler = led_handler,
    .user_ctx = NULL
  };

  // Control handler
  httpd_uri_t control_uri = {
    .uri = "/control",
    .method = HTTP_GET,
    .handler = control_handler,
    .user_ctx = NULL
  };

  // Status handler
  httpd_uri_t status_uri = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = status_handler,
    .user_ctx = NULL
  };

  // Start server
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &stream_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &led_uri);
    httpd_register_uri_handler(camera_httpd, &control_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    Serial.println("HTTP server started");
  }
}

// ==================== HANDLERS ====================

// Stream handler - MJPEG
esp_err_t stream_handler(httpd_req_t* req) {
  camera_fb_t* fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t* _jpg_buf = NULL;
  char part_buf[64];

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
      break;
    }

    if (fb->format != PIXFORMAT_JPEG) {
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!jpeg_converted) {
        Serial.println("JPEG conversion failed");
        res = ESP_FAIL;
        break;
      }
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    // Send frame
    size_t hlen = snprintf(part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char*)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, "\r\n--frame\r\n", 11);
    }

    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }

    if (res != ESP_OK) break;

    // Calculate FPS
    frameCount++;
    unsigned long now = millis();
    if (now - lastFrameTime >= 1000) {
      fps = frameCount * 1000.0 / (now - lastFrameTime);
      frameCount = 0;
      lastFrameTime = now;
    }
  }

  return res;
}

// Capture handler - single JPEG
esp_err_t capture_handler(httpd_req_t* req) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

  esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);

  return res;
}

// LED control handler
esp_err_t led_handler(httpd_req_t* req) {
  char query[32];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    char param[8];
    if (httpd_query_key_value(query, "duty", param, sizeof(param)) == ESP_OK) {
      int duty = atoi(param);
      duty = constrain(duty, 0, 255);
      analogWrite(FLASH_LED_PIN, duty);

      httpd_resp_set_type(req, "application/json");
      char resp[64];
      snprintf(resp, sizeof(resp), "{\"led_duty\":%d}", duty);
      return httpd_resp_send(req, resp, strlen(resp));
    }
  }

  httpd_resp_send_404(req);
  return ESP_FAIL;
}

// Control handler - camera settings
esp_err_t control_handler(httpd_req_t* req) {
  char query[128];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    sensor_t* s = esp_camera_sensor_get();
    if (!s) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }

    char param[16];

    // Framesize
    if (httpd_query_key_value(query, "framesize", param, sizeof(param)) == ESP_OK) {
      if (strcmp(param, "SVGA") == 0) {
        current_framesize = FRAMESIZE_SVGA;
      } else if (strcmp(param, "XGA") == 0) {
        current_framesize = FRAMESIZE_XGA;
      } else if (strcmp(param, "UXGA") == 0) {
        current_framesize = FRAMESIZE_UXGA;
      }
      s->set_framesize(s, current_framesize);
      prefs.putInt("framesize", current_framesize);
    }

    // Quality
    if (httpd_query_key_value(query, "quality", param, sizeof(param)) == ESP_OK) {
      int q = atoi(param);
      current_quality = constrain(q, 10, 63);
      s->set_quality(s, current_quality);
      prefs.putInt("quality", current_quality);
    }

    // Horizontal mirror
    if (httpd_query_key_value(query, "hmirror", param, sizeof(param)) == ESP_OK) {
      current_hmirror = (atoi(param) == 1);
      s->set_hmirror(s, current_hmirror ? 1 : 0);
      prefs.putBool("hmirror", current_hmirror);
    }

    // Vertical flip
    if (httpd_query_key_value(query, "vflip", param, sizeof(param)) == ESP_OK) {
      current_vflip = (atoi(param) == 1);
      s->set_vflip(s, current_vflip ? 1 : 0);
      prefs.putBool("vflip", current_vflip);
    }

    httpd_resp_set_type(req, "application/json");
    char resp[128];
    snprintf(resp, sizeof(resp), "{\"framesize\":%d,\"quality\":%d,\"hmirror\":%d,\"vflip\":%d}",
             current_framesize, current_quality, current_hmirror, current_vflip);
    return httpd_resp_send(req, resp, strlen(resp));
  }

  httpd_resp_send_404(req);
  return ESP_FAIL;
}

// Status handler
esp_err_t status_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");

  char resp[256];
  snprintf(resp, sizeof(resp),
           "{\"uptime\":%lu,\"heap\":%u,\"psram\":%u,\"fps\":%.1f,\"framesize\":%d,\"quality\":%d,\"hmirror\":%d,\"vflip\":%d}",
           millis() / 1000,
           ESP.getFreeHeap(),
           ESP.getFreePsram(),
           fps,
           current_framesize,
           current_quality,
           current_hmirror,
           current_vflip);

  return httpd_resp_send(req, resp, strlen(resp));
}
