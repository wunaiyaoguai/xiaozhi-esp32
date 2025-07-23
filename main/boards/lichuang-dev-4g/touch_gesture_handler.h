#ifndef TOUCH_GESTURE_HANDLER_H
#define TOUCH_GESTURE_HANDLER_H

#include <esp_log.h>
#include <esp_timer.h>
#include <esp_lvgl_port.h>
#include <lvgl.h>
#include <functional>

class TouchGestureHandler {
public:
    TouchGestureHandler();
    ~TouchGestureHandler();
    
    // 设置长按回调函数
    void SetLongPressCallback(std::function<void()> callback);
    
    // 初始化触摸手势处理
    void Initialize();
    
    // 添加事件监听器（在LVGL完全初始化后调用）
    void AddEventListeners();
    
    // 设置检测区域（右上角区域）
    void SetDetectionArea(int x, int y, int width, int height);

private:
    // 触摸事件回调
    static void TouchEventCallback(lv_event_t* e);
    
    // 定时器回调，用于检测长按
    static void LongPressTimerCallback(void* arg);
    
    // 触摸事件处理方法
    void HandleTouchPress(int x, int y);
    void HandleTouchMove(int x, int y);
    void HandleTouchRelease();
    void CancelLongPress();
    
    // 静态实例指针，用于静态回调函数访问
    static TouchGestureHandler* instance_;
    
    // 检测区域配置
    int detect_x_;
    int detect_y_;
    int detect_width_;
    int detect_height_;
    
    // 触摸状态
    bool is_touching_;
    int64_t touch_start_time_;
    int touch_start_x_;
    int touch_start_y_;
    
    // 长按配置
    const int64_t LONG_PRESS_DURATION_MS = 1500;  // 1.5秒长按
    const int TOUCH_TOLERANCE = 20;  // 触摸位置容差
    
    // 回调函数
    std::function<void()> long_press_callback_;
    
    // 定时器
    esp_timer_handle_t long_press_timer_;
    
    // 触摸输入设备
    lv_indev_t* touch_indev_;
};

#endif // TOUCH_GESTURE_HANDLER_H 