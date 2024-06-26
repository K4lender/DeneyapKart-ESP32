#include "DeneyapEsp32cam.h"

#include <Arduino.h>
#include <esp_camera.h>

namespace esp32cam {

CameraClass Camera;

void
CameraClass::cameraInit() {
        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;
        config.pin_d0 = CAMD2;
        config.pin_d1 = CAMD3;
        config.pin_d2 = CAMD4;
        config.pin_d3 = CAMD5;
        config.pin_d4 = CAMD6;
        config.pin_d5 = CAMD7;
        config.pin_d6 = CAMD8;
        config.pin_d7 = CAMD9;
        config.pin_xclk = CAMXC;
        config.pin_pclk = CAMPC;
        config.pin_vsync = CAMV;
        config.pin_href = CAMH;
        config.pin_sscb_sda = CAMSD;
        config.pin_sscb_scl = CAMSC;
        config.pin_pwdn = -1;
        config.pin_reset = -1;
        config.xclk_freq_hz = 15000000;
        config.frame_size = FRAMESIZE_UXGA;
        config.pixel_format = PIXFORMAT_JPEG;
        //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.jpeg_quality = 12;
        config.fb_count = 1;

        //init with high specs to pre-allocate larger buffers                     for larger pre-allocated frame buffer.
        if (config.pixel_format == PIXFORMAT_JPEG) {
            if (psramFound()) {
                config.jpeg_quality = 10;
                config.fb_count = 2;
                config.grab_mode = CAMERA_GRAB_LATEST;
            }
            else {
                // Limit the frame size when PSRAM is not available
                config.frame_size = FRAMESIZE_SVGA;
                config.fb_location = CAMERA_FB_IN_DRAM;
            }
        }
        else {
            // Best option for face detection/recognition
            config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
            config.fb_count = 2;
#endif
        }

        // Camera init
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("Camera init failed with error 0x%x", err);
            return;
        }

        sensor_t* s = esp_camera_sensor_get();
        // Drop down frame size for higher initial frame rate
        s->set_framesize(s, FRAMESIZE_QVGA);
    }


bool
CameraClass::begin(const Config& config) {
  return esp_camera_init(reinterpret_cast<const camera_config_t*>(config.m_cfg)) == ESP_OK;
}

bool
CameraClass::end() {
  return esp_camera_deinit() == ESP_OK;
}

ResolutionList
CameraClass::listResolutions() const {
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor == nullptr) {
    return ResolutionList();
  }

  camera_sensor_info_t* info = esp_camera_sensor_get_info(&sensor->id);
  if (info == nullptr) {
    return ResolutionList();
  }

  return ResolutionList(info->max_size + 1);
}

bool
CameraClass::changeResolution(const Resolution& resolution, int sleepFor) {
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor == nullptr) {
    return false;
  }

  framesize_t frameSize = resolution.as<framesize_t>();
  if (sensor->status.framesize == frameSize) {
    return true;
  }

  if (sensor->set_framesize(sensor, frameSize) != 0) {
    return false;
  }
  if (sleepFor > 0) {
    delay(sleepFor);
  }
  return true;
}

std::unique_ptr<Frame>
CameraClass::capture() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb == nullptr) {
    return nullptr;
  }
  return std::unique_ptr<Frame>(new Frame(fb));
}

int
CameraClass::streamMjpeg(Client& client, const MjpegConfig& cfg) {
  detail::MjpegHeader hdr;
  hdr.prepareResponseHeaders();
  hdr.writeTo(client);

  using Ctrl = detail::MjpegController;
  Ctrl ctrl(cfg);
  while (true) {
    auto act = ctrl.decideAction();
    switch (act) {
      case Ctrl::CAPTURE: {
        ctrl.notifyCapture();
        break;
      }
      case Ctrl::RETURN: {
        ctrl.notifyReturn(capture());
        break;
      }
      case Ctrl::SEND: {
        hdr.preparePartHeader(ctrl.getFrame()->size());
        hdr.writeTo(client);
        ctrl.notifySent(ctrl.getFrame()->writeTo(client, cfg.frameTimeout));
        hdr.preparePartTrailer();
        hdr.writeTo(client);
        break;
      }
      case Ctrl::STOP: {
        client.stop();
        return ctrl.countSentFrames();
      }
      default: {
        delay(act);
        break;
      }
    }
  }
}

} // namespace esp32cam
