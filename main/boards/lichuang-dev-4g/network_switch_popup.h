#ifndef NETWORK_SWITCH_POPUP_H
#define NETWORK_SWITCH_POPUP_H

#include <lvgl.h>
#include <functional>

class NetworkSwitchPopup {
public:
    NetworkSwitchPopup();
    ~NetworkSwitchPopup();
    
    // 显示弹窗
    void Show();
    
    // 延迟初始化UI（在LVGL完全初始化后调用）
    void InitializeUI();
    
    // 隐藏弹窗
    void Hide();
    
    // 设置确认回调函数
    void SetConfirmCallback(std::function<void()> callback);
    
    // 检查弹窗是否显示
    bool IsVisible() const;
    
    // 更新弹窗消息
    void UpdateMessage(const char* current_network, const char* target_network);

private:
    // 创建弹窗UI
    void CreatePopupUI();
    
    // 确认按钮事件回调
    static void ConfirmButtonEventCallback(lv_event_t* e);
    
    // 取消按钮事件回调
    static void CancelButtonEventCallback(lv_event_t* e);
    
    // 弹窗对象
    lv_obj_t* popup_container_;
    lv_obj_t* popup_bg_;
    lv_obj_t* content_panel_;
    lv_obj_t* title_label_;
    lv_obj_t* message_label_;
    lv_obj_t* confirm_btn_;
    lv_obj_t* cancel_btn_;
    
    // 回调函数
    std::function<void()> confirm_callback_;
    
    // 状态
    bool is_visible_;
    bool ui_initialized_;
    
    // 静态实例指针，用于静态回调函数访问
    static NetworkSwitchPopup* instance_;
};

#endif // NETWORK_SWITCH_POPUP_H 