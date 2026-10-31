#include "wifi_qr.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace esphome::wifi_qr {

using tessera::Font;
namespace ui = tessera::ui;

static std::string wifi_escape(const std::string &value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char ch : value) {
    if (ch == '\\' || ch == ';' || ch == ',' || ch == ':' || ch == '"') escaped += '\\';
    escaped += ch;
  }
  return escaped;
}

static uint64_t fingerprint(const std::string &value) {
  uint64_t hash = 1469598103934665603ULL;
  for (unsigned char ch : value) {
    hash ^= ch;
    hash *= 1099511628211ULL;
  }
  return hash;
}

// A QR code and a line of words under it, laid out in the room a tile has: the code as large as the room leaves above
// the line, with its quiet zone in the code's light colour, in the middle. Both tiles draw with it.
class QrView {
 public:
  void create(lv_obj_t *parent, int width, int height) {
    width_ = width;
    height_ = height;
    const int line = ui::line_height(Font::BODY);
    const int gap = ui::px(6);
    const int available = std::max(1, height_ - line - gap);
    const int side = std::max(1, std::min(width_, available));
    const int zone = std::max(4, side / 12);
    const int code = std::max(21, side - 2 * zone);

#if LV_USE_QRCODE
    qr_ = lv_qrcode_create(parent);
    lv_qrcode_set_size(qr_, code);
    lv_obj_set_style_border_width(qr_, zone, 0);
    // The quiet zone is a border around the code and makes the object that much larger: centre the whole of it.
    lv_obj_set_pos(qr_, (width_ - code - 2 * zone) / 2, 0);
    lv_obj_add_flag(qr_, LV_OBJ_FLAG_HIDDEN);
#endif
    status_ = ui::label(parent, Font::BODY, theme::MUTED);
    lv_obj_set_width(status_, width_);
    lv_obj_set_height(status_, line);
    lv_obj_set_pos(status_, 0, height_ - line);
    lv_obj_set_style_text_align(status_, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(status_, LV_LABEL_LONG_DOT);
    paint();
  }

  // The code of `payload` (drawn again only when it changed) and `words` under it; an empty payload hides the code.
  void show(const std::string &payload, const std::string &words) {
#if LV_USE_QRCODE
    if (payload.empty()) {
      lv_obj_add_flag(qr_, LV_OBJ_FLAG_HIDDEN);
      shown_ = false;
    } else {
      const uint64_t next = fingerprint(payload);
      if (!shown_ || next != fingerprint_) {
        lv_qrcode_update(qr_, payload.data(), payload.size());
        fingerprint_ = next;
        lv_obj_remove_flag(qr_, LV_OBJ_FLAG_HIDDEN);
        shown_ = true;
      }
    }
#endif
    ui::set_text(status_, words);
  }

  void paint() {
    ui::set_color(status_, theme::MUTED);
#if LV_USE_QRCODE
    lv_qrcode_set_dark_color(qr_, theme::color(theme::QR_DARK));
    lv_qrcode_set_light_color(qr_, theme::color(theme::QR_LIGHT));
    lv_obj_set_style_border_color(qr_, theme::color(theme::QR_LIGHT), 0);
#endif
  }

 private:
  int width_ = 0;
  int height_ = 0;
  bool shown_ = false;
  uint64_t fingerprint_ = 0;
  lv_obj_t *qr_{};
  lv_obj_t *status_{};
};

class WifiTile : public tessera::Tile {
 public:
  explicit WifiTile(WifiQr *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override { view_.create(c.parent, c.width, c.height); }

  void on_tick(uint32_t) override {
    const std::string &ssid = plugin_->ssid();
    const std::string &password = plugin_->password();
    if (ssid.empty() || password.empty()) {
      view_.show("", plugin_->text("helpers_empty"));
      return;
    }
    view_.show("WIFI:T:WPA;S:" + wifi_escape(ssid) + ";P:" + wifi_escape(password) + ";;", ssid);
  }

  void on_theme() override { view_.paint(); }

 private:
  WifiQr *plugin_;
  QrView view_;
};

// A QR code of any text: the state of the tile's entity (a template sensor or a text helper in Home Assistant, such as
// a link with a value of the day in it, up to 160 bytes), or the text of the tile's option where it has no entity.
class CodeTile : public tessera::Tile {
 public:
  explicit CodeTile(WifiQr *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    fixed_ = c.options["text"] | "";
    caption_ = c.options["caption"] | "";
    view_.create(c.parent, c.width, c.height);
    show(fixed_);
  }

  void on_state(JsonObjectConst data) override {
    const std::string state = data["state"] | "";
    show(state.empty() || state == "unknown" || state == "unavailable" ? fixed_ : state);
  }

  void on_theme() override { view_.paint(); }

 private:
  void show(const std::string &payload) {
    view_.show(payload, payload.empty() ? std::string(plugin_->text("code_empty")) : caption_);
  }

  WifiQr *plugin_;
  QrView view_;
  std::string fixed_, caption_;
};

void WifiQr::setup() {
  add_tile("connect", [this]() { return new WifiTile(this); });
  add_tile("code", [this]() { return new CodeTile(this); });
}

}  // namespace esphome::wifi_qr
