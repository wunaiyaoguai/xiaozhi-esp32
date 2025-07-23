#include "network_switch_popup.h"
#include <esp_log.h>
#include <string>

// 声明项目中使用的字体
LV_FONT_DECLARE(font_puhui_20_4);
LV_FONT_DECLARE(font_awesome_20_4);

static const char* TAG = "NetworkSwitchPopup";

// 静态实例指针
NetworkSwitchPopup* NetworkSwitchPopup::instance_ = nullptr;

NetworkSwitchPopup::NetworkSwitchPopup() 
    : popup_container_(nullptr), popup_bg_(nullptr), content_panel_(nullptr),
      title_label_(nullptr), message_label_(nullptr), confirm_btn_(nullptr), cancel_btn_(nullptr),
      is_visible_(false), ui_initialized_(false) {
    instance_ = this;
    ESP_LOGI(TAG, "NetworkSwitchPopup constructor called");
    // 不要在构造函数中创建UI，等LVGL完全初始化后再创建
}

NetworkSwitchPopup::~NetworkSwitchPopup() {
    if (popup_container_) {
        lv_obj_del(popup_container_);
    }
    instance_ = nullptr;
}

void NetworkSwitchPopup::InitializeUI() {
    if (ui_initialized_) {
        return;  // 避免重复初始化
    }
    CreatePopupUI();
    ui_initialized_ = true;
}

void NetworkSwitchPopup::CreatePopupUI() {
    // 创建弹窗容器（全屏覆盖）
    popup_container_ = lv_obj_create(lv_screen_active());
    lv_obj_set_size(popup_container_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(popup_container_, 0, 0);
    lv_obj_clear_flag(popup_container_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(popup_container_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(popup_container_, 128, 0);  // 半透明背景
    lv_obj_set_style_border_width(popup_container_, 0, 0);
    lv_obj_add_flag(popup_container_, LV_OBJ_FLAG_HIDDEN);  // 初始隐藏
    
    // 添加背景点击事件（点击背景即取消）
    lv_obj_add_event_cb(popup_container_, CancelButtonEventCallback, LV_EVENT_CLICKED, this);
    ESP_LOGI(TAG, "🌐 Background click event added to popup container %p", popup_container_);
    
    // 创建内容面板（重新设计尺寸和布局）
    content_panel_ = lv_obj_create(popup_container_);
    lv_obj_set_size(content_panel_, 240, 150);
    lv_obj_center(content_panel_);
    lv_obj_clear_flag(content_panel_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content_panel_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(content_panel_, 255, 0);
    lv_obj_set_style_border_color(content_panel_, lv_color_hex(0x0066cc), 0);
    lv_obj_set_style_border_width(content_panel_, 2, 0);
    lv_obj_set_style_radius(content_panel_, 8, 0);
    lv_obj_set_style_shadow_width(content_panel_, 15, 0);
    lv_obj_set_style_shadow_opa(content_panel_, 30, 0);
    
    // 防止内容面板阻止背景点击事件
    lv_obj_add_flag(content_panel_, LV_OBJ_FLAG_EVENT_BUBBLE);
    
    // 创建标题标签
    title_label_ = lv_label_create(content_panel_);
    lv_label_set_text(title_label_, "网络切换确认");
    lv_obj_set_style_text_font(title_label_, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_align(title_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(title_label_, 0, 8);
    lv_obj_set_width(title_label_, 240);
    
    // 创建消息标签（改善布局，避免文字重叠）
    message_label_ = lv_label_create(content_panel_);
    lv_obj_set_style_text_align(message_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(message_label_, &font_awesome_20_4, 0);
    lv_obj_set_style_text_color(message_label_, lv_color_hex(0x666666), 0);
    lv_obj_set_pos(message_label_, 10, 35);
    lv_obj_set_size(message_label_, 220, 70);
    lv_label_set_long_mode(message_label_, LV_LABEL_LONG_WRAP);
    
    // 只创建确定按钮，居中显示
    confirm_btn_ = lv_btn_create(content_panel_);
    lv_obj_set_size(confirm_btn_, 100, 35);
    lv_obj_set_pos(confirm_btn_, 70, 110);  // 居中位置
    lv_obj_set_style_bg_color(confirm_btn_, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_bg_color(confirm_btn_, lv_color_hex(0x1976D2), LV_STATE_PRESSED);
    lv_obj_set_style_radius(confirm_btn_, 6, 0);
    
    // 添加事件回调并记录日志
    lv_obj_add_event_cb(confirm_btn_, ConfirmButtonEventCallback, LV_EVENT_CLICKED, this);
    ESP_LOGI(TAG, "✅ Confirm button created at %p with event callback", confirm_btn_);
    
    lv_obj_t* confirm_label = lv_label_create(confirm_btn_);
    lv_label_set_text(confirm_label, "确定切换");
    lv_obj_set_style_text_font(confirm_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(confirm_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(confirm_label);
    
    // 移除取消按钮，现在点击背景即取消
    cancel_btn_ = nullptr;
    
    ESP_LOGI(TAG, "Network switch popup UI created");
}

void NetworkSwitchPopup::Show() {
    ESP_LOGI(TAG, "Show() called, ui_initialized=%d, is_visible=%d", ui_initialized_, is_visible_);
    
    // 确保UI已初始化
    if (!ui_initialized_) {
        ESP_LOGI(TAG, "Initializing UI...");
        InitializeUI();
    }
    
    if (popup_container_ && !is_visible_) {
        lv_obj_clear_flag(popup_container_, LV_OBJ_FLAG_HIDDEN);
        is_visible_ = true;
        ESP_LOGI(TAG, "🎉 Network switch popup shown successfully");
        ESP_LOGI(TAG, "📍 Instructions: Click '确定切换' to confirm, or click background to cancel");
    } else {
        ESP_LOGW(TAG, "Cannot show popup: container=%p, is_visible=%d", popup_container_, is_visible_);
    }
}

void NetworkSwitchPopup::Hide() {
    if (popup_container_ && is_visible_) {
        lv_obj_add_flag(popup_container_, LV_OBJ_FLAG_HIDDEN);
        is_visible_ = false;
        ESP_LOGI(TAG, "Network switch popup hidden");
    }
}

void NetworkSwitchPopup::SetConfirmCallback(std::function<void()> callback) {
    confirm_callback_ = callback;
}

bool NetworkSwitchPopup::IsVisible() const {
    return is_visible_;
}

void NetworkSwitchPopup::ConfirmButtonEventCallback(lv_event_t* e) {
    if (!instance_) {
        ESP_LOGW(TAG, "ConfirmButtonEventCallback: No instance");
        return;
    }
    
    lv_event_code_t code = lv_event_get_code(e);
    ESP_LOGI(TAG, "🟢 Confirm button event: code=%d", code);
    
    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "🚀 Confirm button clicked - processing network switch");
        
        // 隐藏弹窗
        instance_->Hide();
        
        // 调用确认回调
        if (instance_->confirm_callback_) {
            ESP_LOGI(TAG, "Calling confirm callback...");
            instance_->confirm_callback_();
        } else {
            ESP_LOGW(TAG, "No confirm callback set");
        }
    }
}

void NetworkSwitchPopup::CancelButtonEventCallback(lv_event_t* e) {
    if (!instance_) {
        ESP_LOGW(TAG, "CancelButtonEventCallback: No instance");
        return;
    }
    
    lv_event_code_t code = lv_event_get_code(e);
    ESP_LOGI(TAG, "🔴 Cancel event: code=%d", code);
    
    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "🚫 Cancel clicked (background or cancel button) - hiding popup");
        
        // 隐藏弹窗
        instance_->Hide();
    }
}

void NetworkSwitchPopup::UpdateMessage(const char* current_network, const char* target_network) {
    // 确保UI已初始化
    if (!ui_initialized_) {
        InitializeUI();
    }
    
    if (!message_label_) {
        return;
    }
    
    // 构建新的消息文本（简化布局，避免文字重叠）
    std::string message = "确定要切换网络模式吗？\n\n";
    message += current_network;
    message += " → ";
    message += target_network;
    message += "\n\n设备将重启";
    
    lv_label_set_text(message_label_, message.c_str());
    ESP_LOGI(TAG, "Updated popup message: %s -> %s", current_network, target_network);
} 