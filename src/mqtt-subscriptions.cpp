/*
 * ESP32 Template
 * Definition of MQTT subscriptions
 */
#include "mqtt-ota-config.h"
#include "user-config.h"

//
// MqttSubscriptions is a dataset with all configuration information required
// to subscribe to configured topics and handle the received messages
// Results of decoded messages will be stored into a configured global variable using a pointer
//
// Configuration:
// .Topic: String of topic to subscribe to
// .Type:   0 = bool (expected message "on" or "off")
//          1 = integer (int)
//          2 = float
// .Subscribed: flag, true if successfully subscribed to topic
// .MsgRcvd: flag, true if a message has been received for subscribed topic
// .[Bool|Int|Float]Ptr: Pointer to a global var (according to "Type") where the decoded message info will be stored 
//

const int SubscribedTopicCnt = 22; // Overall amount of topics to subscribe to

MqttSubCfg MqttSubscriptions[SubscribedTopicCnt]={
    // OTA Topics
    {.Topic = ota_topic, .Type = 0, .Subscribed = false, .MsgRcvd = false, .BoolPtr = &OTAupdate },
    {.Topic = otaInProgress_topic, .Type = 0, .Subscribed = false, .MsgRcvd = false, .BoolPtr = &OtaInProgress },
    // SSR1
    {.Topic = t_Ctrl_Cfg_SSR1_setState, .Type = 0, .Subscribed = false, .MsgRcvd = false, .BoolPtr = &SSR1.setState },
    {.Topic = t_Ctrl_Cfg_SSR1_Auto, .Type = 0, .Subscribed = false, .MsgRcvd = false, .BoolPtr = &SSR1.Auto },
    {.Topic = t_Ctrl_Cfg_SSR1_LPOnSOC, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR1.LPOnSOC },
    {.Topic = t_Ctrl_Cfg_SSR1_LPOffSOC, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR1.LPOffSOC },
    {.Topic = t_Ctrl_Cfg_SSR1_HPOnSOC, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR1.HPOnSOC },
    {.Topic = t_Ctrl_Cfg_SSR1_HPOffSOC, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR1.HPOffSOC },
    // SSR3
    {.Topic = t_Ctrl_Cfg_SSR3_setState, .Type = 0, .Subscribed = false, .MsgRcvd = false, .BoolPtr = &SSR3.setState },
    {.Topic = t_Ctrl_Cfg_SSR3_Auto, .Type = 0, .Subscribed = false, .MsgRcvd = false, .BoolPtr = &SSR3.Auto },
    {.Topic = t_Ctrl_Cfg_SSR3_LPOnSOC, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR3.LPOnSOC },
    {.Topic = t_Ctrl_Cfg_SSR3_LPOffSOC, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR3.LPOffSOC },
    {.Topic = t_Ctrl_Cfg_SSR3_HPOnSOC, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR3.HPOnSOC },
    {.Topic = t_Ctrl_Cfg_SSR3_HPOffSOC, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR3.HPOffSOC },
    // SSR2
    {.Topic = t_Ctrl_Cfg_SSR2_setState, .Type = 0, .Subscribed = false, .MsgRcvd = false, .BoolPtr = &SSR2.setState },
    {.Topic = t_Ctrl_Cfg_SSR2_Auto, .Type = 0, .Subscribed = false, .MsgRcvd = false, .BoolPtr = &SSR2.Auto },
    {.Topic = t_Ctrl_Cfg_SSR2_CVOn, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR2.CVOn },
    {.Topic = t_Ctrl_Cfg_SSR2_CVOff, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR2.CVOff },
    {.Topic = t_Ctrl_Cfg_SSR2_CdiffOn, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR2.CdiffOn },
    {.Topic = t_Ctrl_Cfg_SSR2_AlrmCdiff, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &SSR2.AlrmCdiff },
    // PV
    {.Topic = t_Ctrl_Cfg_PV_LowPPV, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &PV.LowPPV },
    {.Topic = t_Ctrl_Cfg_PV_HighPPV, .Type = 1, .Subscribed = false, .MsgRcvd = false, .IntPtr = &PV.HighPPV }
};
