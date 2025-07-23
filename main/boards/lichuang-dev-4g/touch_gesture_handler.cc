#include "touch_gesture_handler.h"

static const char* TAG = "TouchGestureHandler";

// 静态实例指针
TouchGestureHandler* TouchGestureHandler::instance_ = nullptr;

TouchGestureHandler::TouchGestureHandler() 
    : detect_x_(0), detect_y_(0), detect_width_(0), detect_height_(0),
      is_touching_(false), touch_start_time_(0), touch_start_x_(0), touch_start_y_(0),
      long_press_timer_(nullptr), touch_indev_(nullptr) {
    instance_ = this;
    ESP_LOGI(TAG, "TouchGestureHandler constructor called");
}

TouchGestureHandler::~TouchGestureHandler() {
    if (long_press_timer_) {
        esp_timer_delete(long_press_timer_);
    }
    instance_ = nullptr;
}

void TouchGestureHandler::SetLongPressCallback(std::function<void()> callback) {
    long_press_callback_ = callback;
    ESP_LOGI(TAG, "Long press callback set successfully");
}

void TouchGestureHandler::SetDetectionArea(int x, int y, int width, int height) {
    detect_x_ = x;
    detect_y_ = y;
    detect_width_ = width;
    detect_height_ = height;
    ESP_LOGI(TAG, "Detection area set to (%d, %d, %d, %d)", x, y, width, height);
    ESP_LOGI(TAG, "Right top corner detection zone: X=%d-%d, Y=%d-%d", x, x+width, y, y+height);
}

void TouchGestureHandler::Initialize() {
    ESP_LOGI(TAG, "TouchGestureHandler::Initialize() called");
    
    // 创建长按检测定时器
    esp_timer_create_args_t timer_args = {
        .callback = LongPressTimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "long_press_timer",
        .skip_unhandled_events = true,
    };
    
    esp_err_t ret = esp_timer_create(&timer_args, &long_press_timer_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create long press timer: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "Touch gesture handler timer created successfully, will add event callbacks later");
}

void TouchGestureHandler::AddEventListeners() {
    // 获取触摸输入设备
    touch_indev_ = lv_indev_get_next(nullptr);
    int indev_count = 0;
    
    ESP_LOGI(TAG, "Scanning for input devices...");
    lv_indev_t* temp_indev = lv_indev_get_next(nullptr);
    while (temp_indev) {
        indev_count++;
        ESP_LOGI(TAG, "Found input device %d, type=%d", indev_count, lv_indev_get_type(temp_indev));
        if (lv_indev_get_type(temp_indev) == LV_INDEV_TYPE_POINTER) {
            touch_indev_ = temp_indev;
            ESP_LOGI(TAG, "Found touch input device at index %d", indev_count);
        }
        temp_indev = lv_indev_get_next(temp_indev);
    }
    
    if (!touch_indev_) {
        ESP_LOGE(TAG, "No touch input device found after scanning %d devices", indev_count);
        return;
    }
    
    // 直接监听输入设备事件，而不是屏幕对象事件
    ESP_LOGI(TAG, "Adding touch event callbacks to input device %p", touch_indev_);
    lv_indev_add_event_cb(touch_indev_, TouchEventCallback, LV_EVENT_PRESSED, this);
    lv_indev_add_event_cb(touch_indev_, TouchEventCallback, LV_EVENT_PRESSING, this);
    lv_indev_add_event_cb(touch_indev_, TouchEventCallback, LV_EVENT_RELEASED, this);
    lv_indev_add_event_cb(touch_indev_, TouchEventCallback, LV_EVENT_CLICKED, this);
    lv_indev_add_event_cb(touch_indev_, TouchEventCallback, LV_EVENT_PRESS_LOST, this);
    
    // 同时也添加到屏幕对象，以防万一
    lv_obj_t* screen = lv_screen_active();
    if (screen) {
        ESP_LOGI(TAG, "Also adding touch event callbacks to screen %p", screen);
        lv_obj_add_event_cb(screen, TouchEventCallback, LV_EVENT_PRESSED, this);
        lv_obj_add_event_cb(screen, TouchEventCallback, LV_EVENT_PRESSING, this);
        lv_obj_add_event_cb(screen, TouchEventCallback, LV_EVENT_RELEASED, this);
        lv_obj_add_event_cb(screen, TouchEventCallback, LV_EVENT_CLICKED, this);
    }
    
    ESP_LOGI(TAG, "Touch gesture event listeners added successfully");
    ESP_LOGI(TAG, "Detection area: (%d,%d) size (%d,%d)", detect_x_, detect_y_, detect_width_, detect_height_);
}

void TouchGestureHandler::TouchEventCallback(lv_event_t* e) {
    if (!instance_) {
        ESP_LOGW(TAG, "TouchEventCallback: No instance");
        return;
    }
    
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev = lv_indev_active();
    
    if (!indev) {
        ESP_LOGW(TAG, "TouchEventCallback: No active input device");
        return;
    }
    
    if (lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) {
        ESP_LOGD(TAG, "TouchEventCallback: Not a pointer device, type=%d", lv_indev_get_type(indev));
        return;
    }
    
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    
    ESP_LOGD(TAG, "Touch event: code=%d, pos=(%ld,%ld)", code, (long)point.x, (long)point.y);
    
    switch (code) {
        case LV_EVENT_PRESSED:
            ESP_LOGI(TAG, "Touch PRESSED at (%ld,%ld)", (long)point.x, (long)point.y);
            instance_->HandleTouchPress(point.x, point.y);
            break;
            
        case LV_EVENT_PRESSING:
            instance_->HandleTouchMove(point.x, point.y);
            break;
            
        case LV_EVENT_RELEASED:
            ESP_LOGI(TAG, "Touch RELEASED");
            instance_->HandleTouchRelease();
            break;
            
        default:
            ESP_LOGD(TAG, "Unhandled touch event: %d", code);
            break;
    }
}

void TouchGestureHandler::HandleTouchPress(int x, int y) {
    // 记录所有触摸位置用于调试
    ESP_LOGI(TAG, "HandleTouchPress: pos=(%d,%d), detection area=(%d,%d,%d,%d)", 
             x, y, detect_x_, detect_y_, detect_width_, detect_height_);
    
    // 检查是否在检测区域内
    if (x >= detect_x_ && x <= (detect_x_ + detect_width_) &&
        y >= detect_y_ && y <= (detect_y_ + detect_height_)) {
        
        is_touching_ = true;
        touch_start_time_ = esp_timer_get_time() / 1000;  // 转换为毫秒
        touch_start_x_ = x;
        touch_start_y_ = y;
        
        // 启动长按定时器
        esp_timer_start_once(long_press_timer_, LONG_PRESS_DURATION_MS * 1000);  // 转换为微秒
        
        ESP_LOGI(TAG, "🎯 Touch press detected in RIGHT TOP CORNER at (%d, %d) - starting long press timer", x, y);
    } else {
        ESP_LOGD(TAG, "Touch outside detection area: (%d,%d) not in (%d-%d, %d-%d)", 
                 x, y, detect_x_, detect_x_ + detect_width_, detect_y_, detect_y_ + detect_height_);
    }
}

void TouchGestureHandler::HandleTouchMove(int x, int y) {
    if (!is_touching_) {
        return;
    }
    
    // 检查触摸位置是否偏移太大
    int dx = abs(x - touch_start_x_);
    int dy = abs(y - touch_start_y_);
    
    if (dx > TOUCH_TOLERANCE || dy > TOUCH_TOLERANCE) {
        // 触摸位置偏移太大，取消长按检测
        CancelLongPress();
        ESP_LOGD(TAG, "Touch moved too far, canceling long press");
    }
}

void TouchGestureHandler::HandleTouchRelease() {
    if (is_touching_) {
        CancelLongPress();
        ESP_LOGD(TAG, "Touch released");
    }
}

void TouchGestureHandler::CancelLongPress() {
    is_touching_ = false;
    esp_timer_stop(long_press_timer_);
}

void TouchGestureHandler::LongPressTimerCallback(void* arg) {
    TouchGestureHandler* handler = static_cast<TouchGestureHandler*>(arg);
    
    ESP_LOGI(TAG, "🔥 LongPressTimerCallback triggered! handler=%p, is_touching=%d", 
             handler, handler ? handler->is_touching_ : -1);
    
    if (handler && handler->is_touching_ && handler->long_press_callback_) {
        ESP_LOGI(TAG, "🚀 Long press detected in right top corner, triggering callback");
        handler->long_press_callback_();
        handler->is_touching_ = false;
    } else {
        ESP_LOGW(TAG, "Long press callback not triggered: handler=%p, is_touching=%d, callback=%p", 
                 handler, handler ? handler->is_touching_ : -1, 
                 handler && handler->long_press_callback_ ? (void*)1 : (void*)0);
    }
} 