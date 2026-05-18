#ifndef CAN_FRAMES_H
#define CAN_FRAMES_H
#include <stdint.h>

// ============================================================================
// CAN FD 帧结构体 — 用位域直接映射 DBC 信号
// ============================================================================
// 使用方法:
//   1. 中断里收到 64 字节 CAN FD 帧 → memcpy(frame.raw, rx_buf, 64)
//   2. 读取 CAN ID → frame.id
//   3. 根据 ID 访问对应 union: frame.canfd_0x2cf.VIU_AL_Welcome 等
//
// 注意: 位域布局依赖编译器 ABI (ARM GCC / IAR 已验证)

typedef struct {
    // ---- CAN ID (bytes 0..3 LE) + raw buffer ----
    union {
        struct {
            uint32_t id;       // CAN ID (uint32 LE)
            uint8_t  payload[60];
        };
        uint8_t raw[64];
    };

    // ---- 0x123 测试帧 ----
    union {
        struct {
            uint8_t R : 8;
            uint8_t G : 8;
            uint8_t B : 8;
            uint8_t L : 8;
            uint8_t F : 8;
        };
        uint8_t _test_0x123_data[64];
    } test_0x123;

    // ========================================================================
    // 0x2CF VIU_ALCtrl — VIU → ALC 主控制帧 (32 bytes, 100ms)
    // ========================================================================
    union {
        struct {
            // Ceiling Lamp (byte 0..3)
            uint8_t VIU_AL_CeilingLampColorSet_Red   : 8;
            uint8_t VIU_AL_CeilingLampColorSet_Green : 8;
            uint8_t VIU_AL_CeilingLampColorSet_Blue  : 8;
            uint8_t VIU_AL_CeilingLampBriSet         : 7;
            uint8_t VIU_AL_CeilingLampBriModSet      : 1;

            // Hi Lamp (byte 4..7)
            uint8_t VIU_AL_HiLampColorSet_Red   : 8;
            uint8_t VIU_AL_HiLampColorSet_Green : 8;
            uint8_t VIU_AL_HiLampColorSet_Blue  : 8;
            uint8_t VIU_AL_HiLampBriSet         : 7;
            uint8_t VIU_AL_HiLampBriModSet      : 1;

            // Neut Lamp (byte 8..11)
            uint8_t VIU_AL_NeutLampColorSet_Red   : 8;
            uint8_t VIU_AL_NeutLampColorSet_Green : 8;
            uint8_t VIU_AL_NeutLampColorSet_Blue  : 8;
            uint8_t VIU_AL_NeutLampBriSet         : 7;
            uint8_t VIU_AL_NeutLampBriModSet      : 1;

            // Foot Lamp (byte 12..15)
            uint8_t VIU_AL_FootLampColorSet_Red   : 8;
            uint8_t VIU_AL_FootLampColorSet_Green : 8;
            uint8_t VIU_AL_FootLampColorSet_Blue  : 8;
            uint8_t VIU_AL_FootLampBriSet         : 7;
            uint8_t VIU_AL_FootLampBriModSet      : 1;

            // Mode / AC (byte 16)
            uint8_t VIU_AL_AtmThemeMod : 4;
            uint8_t VIU_AL_NapMod      : 4;

            // AC linkage (byte 17)
            uint8_t VIU_AL_FLAC : 2;
            uint8_t VIU_AL_FRAC : 2;
            uint8_t VIU_AL_RLAC : 2;
            uint8_t VIU_AL_RRAC : 2;

            // Motion / Voice (byte 18)
            uint8_t VIU_AL_FragranceMotion        : 4;
            uint8_t VIU_AL_VoiceInteractIndcrSt   : 4;

            // DoorWarnArea / VoiceArea / StartMotion (byte 19)
            uint8_t VIU_AL_DoorWarnArea          : 3;
            uint8_t VIU_AL_VoiceInteractIndcrArea : 3;
            uint8_t VIU_AL_StartMotion           : 2;

            // (byte 20)
            uint8_t VIU_AL_RearWarn   : 2;
            uint8_t VIU_AL_DoorWarn   : 2;
            uint8_t VIU_AL_DrivingMod : 4;

            // (byte 21)
            uint8_t reserved1         : 1;
            uint8_t VIU_AL_PowerSt    : 2;
            uint8_t VIU_AL_Welcome    : 5;

            // (byte 22)
            uint8_t reserved2        : 2;
            uint8_t VIU_AL_DayNightSt : 2;
            uint8_t reserved3        : 4;

            // RearWarn per door (byte 23)
            uint8_t VIU_AL_RRRearWarn : 2;
            uint8_t VIU_AL_RLRearWarn : 2;
            uint8_t VIU_AL_FRRearWarn : 2;
            uint8_t VIU_AL_FLRearWarn : 2;

            // DoorWarn per door (byte 24)
            uint8_t VIU_AL_RRDoorWarn : 2;
            uint8_t VIU_AL_RLDoorWarn : 2;
            uint8_t VIU_AL_FRDoorWarn : 2;
            uint8_t VIU_AL_FLDoorWarn : 2;

            // VideoAmbLi area enable + Tweeter (byte 25)
            uint8_t VIU_AL_VideoAmbLiAreaRR  : 1;
            uint8_t VIU_AL_VideoAmbLiAreaRL  : 1;
            uint8_t VIU_AL_VideoAmbLiAreaMR  : 1;
            uint8_t VIU_AL_VideoAmbLiAreaML  : 1;
            uint8_t VIU_AL_VideoAmbLiAreaFR  : 1;
            uint8_t VIU_AL_VideoAmbLiAreaFL  : 1;
            uint8_t VIU_AL_TwetterLampEnable : 2;

            // VideoAmbLi Color (bytes 26..28)
            uint8_t VIU_AL_VideoAmbLiColorSet_Red   : 8;
            uint8_t VIU_AL_VideoAmbLiColorSet_Green : 8;
            uint8_t VIU_AL_VideoAmbLiColorSet_Blue  : 8;
        };
        uint8_t _0x2cf_data[64];
    } canfd_0x2cf;

    // ========================================================================
    // 0x2CE AL_ALSts — ALC → VIU 状态反馈 (32 bytes, 100ms)
    // ========================================================================
    union {
        struct {
            uint8_t AL_VIU_Faultfeedback : 4;
            uint8_t AL_VIU_AtmThemeMod   : 4;
        };
        uint8_t _0x2ce_data[64];
    } canfd_0x2ce;

    // ========================================================================
    // 0x1E9 VIU_ALMusicFollowCtrl1 — 音乐随动 1-4
    // ========================================================================
    union {
        struct {
            uint8_t reserved1     : 3;
            uint8_t VIU_AL_PreDishesArea1 : 5;
            uint8_t VIU_AL_PreDishesMod1  : 2;
            uint8_t reserved2     : 2;
            uint8_t reserved3[7];
            uint8_t VIU_AL_ColorSet_R1 : 8;
            uint8_t VIU_AL_ColorSet_G1 : 8;
            uint8_t VIU_AL_ColorSet_B1 : 8;
            uint8_t VIU_AL_BriSet1     : 7;
            uint8_t VIU_AL_BriModSet1  : 1;

            uint8_t reserved4     : 3;
            uint8_t VIU_AL_PreDishesArea2 : 5;
            uint8_t VIU_AL_PreDishesMod2  : 2;
            uint8_t reserved5     : 2;
            uint8_t reserved6[7];
            uint8_t VIU_AL_ColorSet_R2 : 8;
            uint8_t VIU_AL_ColorSet_G2 : 8;
            uint8_t VIU_AL_ColorSet_B2 : 8;
            uint8_t VIU_AL_BriSet2     : 7;
            uint8_t VIU_AL_BriModSet2  : 1;

            uint8_t reserved7     : 3;
            uint8_t VIU_AL_PreDishesArea3 : 5;
            uint8_t VIU_AL_PreDishesMod3  : 2;
            uint8_t reserved8     : 2;
            uint8_t reserved9[7];
            uint8_t VIU_AL_ColorSet_R3 : 8;
            uint8_t VIU_AL_ColorSet_G3 : 8;
            uint8_t VIU_AL_ColorSet_B3 : 8;
            uint8_t VIU_AL_BriSet3     : 7;
            uint8_t VIU_AL_BriModSet3  : 1;

            uint8_t reserved10     : 3;
            uint8_t VIU_AL_PreDishesArea4 : 5;
            uint8_t VIU_AL_PreDishesMod4  : 2;
            uint8_t reserved11     : 2;
            uint8_t reserved12[7];
            uint8_t VIU_AL_ColorSet_R4 : 8;
            uint8_t VIU_AL_ColorSet_G4 : 8;
            uint8_t VIU_AL_ColorSet_B4 : 8;
            uint8_t VIU_AL_BriSet4     : 7;
            uint8_t VIU_AL_BriModSet4  : 1;
        };
        uint8_t _0x1e9_data[64];
    } canfd_0x1e9;

    // ========================================================================
    // 0x1EA VIU_ALMusicFollowCtrl2 — 音乐随动 5-8
    // ========================================================================
    union {
        struct {
            uint8_t _rsv0 : 3; uint8_t VIU_AL_PreDishesArea5 : 5;
            uint8_t VIU_AL_PreDishesMod5 : 2; uint8_t _rsv1 : 2; uint8_t _rsv2[7];
            uint8_t VIU_AL_ColorSet_R5 : 8; uint8_t VIU_AL_ColorSet_G5 : 8;
            uint8_t VIU_AL_ColorSet_B5 : 8; uint8_t VIU_AL_BriSet5 : 7;
            uint8_t VIU_AL_BriModSet5 : 1;

            uint8_t _rsv3 : 3; uint8_t VIU_AL_PreDishesArea6 : 5;
            uint8_t VIU_AL_PreDishesMod6 : 2; uint8_t _rsv4 : 2; uint8_t _rsv5[7];
            uint8_t VIU_AL_ColorSet_R6 : 8; uint8_t VIU_AL_ColorSet_G6 : 8;
            uint8_t VIU_AL_ColorSet_B6 : 8; uint8_t VIU_AL_BriSet6 : 7;
            uint8_t VIU_AL_BriModSet6 : 1;

            uint8_t _rsv6 : 3; uint8_t VIU_AL_PreDishesArea7 : 5;
            uint8_t VIU_AL_PreDishesMod7 : 2; uint8_t _rsv7 : 2; uint8_t _rsv8[7];
            uint8_t VIU_AL_ColorSet_R7 : 8; uint8_t VIU_AL_ColorSet_G7 : 8;
            uint8_t VIU_AL_ColorSet_B7 : 8; uint8_t VIU_AL_BriSet7 : 7;
            uint8_t VIU_AL_BriModSet7 : 1;

            uint8_t _rsv9 : 3; uint8_t VIU_AL_PreDishesArea8 : 5;
            uint8_t VIU_AL_PreDishesMod8 : 2; uint8_t _rsv10 : 2; uint8_t _rsv11[7];
            uint8_t VIU_AL_ColorSet_R8 : 8; uint8_t VIU_AL_ColorSet_G8 : 8;
            uint8_t VIU_AL_ColorSet_B8 : 8; uint8_t VIU_AL_BriSet8 : 7;
            uint8_t VIU_AL_BriModSet8 : 1;
        };
        uint8_t _0x1ea_data[64];
    } canfd_0x1ea;

    // ========================================================================
    // 0x1EB VIU_ALMusicFollowCtrl3 — 音乐随动 9-12
    // ========================================================================
    union {
        struct {
            uint8_t _r0 : 3; uint8_t VIU_AL_PreDishesArea9  : 5;
            uint8_t VIU_AL_PreDishesMod9  : 2; uint8_t _r1 : 2; uint8_t _r2[7];
            uint8_t VIU_AL_ColorSet_R9  : 8; uint8_t VIU_AL_ColorSet_G9  : 8;
            uint8_t VIU_AL_ColorSet_B9  : 8; uint8_t VIU_AL_BriSet9  : 7;
            uint8_t VIU_AL_BriModSet9  : 1;

            uint8_t _r3 : 3; uint8_t VIU_AL_PreDishesArea10 : 5;
            uint8_t VIU_AL_PreDishesMod10 : 2; uint8_t _r4 : 2; uint8_t _r5[7];
            uint8_t VIU_AL_ColorSet_R10 : 8; uint8_t VIU_AL_ColorSet_G10 : 8;
            uint8_t VIU_AL_ColorSet_B10 : 8; uint8_t VIU_AL_BriSet10 : 7;
            uint8_t VIU_AL_BriModSet10 : 1;

            uint8_t _r6 : 3; uint8_t VIU_AL_PreDishesArea11 : 5;
            uint8_t VIU_AL_PreDishesMod11 : 2; uint8_t _r7 : 2; uint8_t _r8[7];
            uint8_t VIU_AL_ColorSet_R11 : 8; uint8_t VIU_AL_ColorSet_G11 : 8;
            uint8_t VIU_AL_ColorSet_B11 : 8; uint8_t VIU_AL_BriSet11 : 7;
            uint8_t VIU_AL_BriModSet11 : 1;

            uint8_t _r9 : 3; uint8_t VIU_AL_PreDishesArea12 : 5;
            uint8_t VIU_AL_PreDishesMod12 : 2; uint8_t _r10 : 2; uint8_t _r11[7];
            uint8_t VIU_AL_ColorSet_R12 : 8; uint8_t VIU_AL_ColorSet_G12 : 8;
            uint8_t VIU_AL_ColorSet_B12 : 8; uint8_t VIU_AL_BriSet12 : 7;
            uint8_t VIU_AL_BriModSet12 : 1;
        };
        uint8_t _0x1eb_data[64];
    } canfd_0x1eb;

    // ========================================================================
    // 0x1EC VIU_ALMusicFollowCtrl4 — 音乐随动 13-16
    // ========================================================================
    union {
        struct {
            uint8_t _s0 : 3; uint8_t VIU_AL_PreDishesArea13 : 5;
            uint8_t VIU_AL_PreDishesMod13 : 2; uint8_t _s1 : 2; uint8_t _s2[7];
            uint8_t VIU_AL_ColorSet_R13 : 8; uint8_t VIU_AL_ColorSet_G13 : 8;
            uint8_t VIU_AL_ColorSet_B13 : 8; uint8_t VIU_AL_BriSet13 : 7;
            uint8_t VIU_AL_BriModSet13 : 1;

            uint8_t _s3 : 3; uint8_t VIU_AL_PreDishesArea14 : 5;
            uint8_t VIU_AL_PreDishesMod14 : 2; uint8_t _s4 : 2; uint8_t _s5[7];
            uint8_t VIU_AL_ColorSet_R14 : 8; uint8_t VIU_AL_ColorSet_G14 : 8;
            uint8_t VIU_AL_ColorSet_B14 : 8; uint8_t VIU_AL_BriSet14 : 7;
            uint8_t VIU_AL_BriModSet14 : 1;

            uint8_t _s6 : 3; uint8_t VIU_AL_PreDishesArea15 : 5;
            uint8_t VIU_AL_PreDishesMod15 : 2; uint8_t _s7 : 2; uint8_t _s8[7];
            uint8_t VIU_AL_ColorSet_R15 : 8; uint8_t VIU_AL_ColorSet_G15 : 8;
            uint8_t VIU_AL_ColorSet_B15 : 8; uint8_t VIU_AL_BriSet15 : 7;
            uint8_t VIU_AL_BriModSet15 : 1;

            uint8_t _s9 : 3; uint8_t VIU_AL_PreDishesArea16 : 5;
            uint8_t VIU_AL_PreDishesMod16 : 2; uint8_t _s10 : 2; uint8_t _s11[7];
            uint8_t VIU_AL_ColorSet_R16 : 8; uint8_t VIU_AL_ColorSet_G16 : 8;
            uint8_t VIU_AL_ColorSet_B16 : 8; uint8_t VIU_AL_BriSet16 : 7;
            uint8_t VIU_AL_BriModSet16 : 1;
        };
        uint8_t _0x1ec_data[64];
    } canfd_0x1ec;

    // ========================================================================
    // 0x1ED VIU_ALMusicFollowCtrl5 — 音乐随动 17-19
    // ========================================================================
    union {
        struct {
            uint8_t _t0 : 3; uint8_t VIU_AL_PreDishesArea17 : 5;
            uint8_t VIU_AL_PreDishesMod17 : 2; uint8_t _t1 : 2; uint8_t _t2[7];
            uint8_t VIU_AL_ColorSet_R17 : 8; uint8_t VIU_AL_ColorSet_G17 : 8;
            uint8_t VIU_AL_ColorSet_B17 : 8; uint8_t VIU_AL_BriSet17 : 7;
            uint8_t VIU_AL_BriModSet17 : 1;

            uint8_t _t3 : 3; uint8_t VIU_AL_PreDishesArea18 : 5;
            uint8_t VIU_AL_PreDishesMod18 : 2; uint8_t _t4 : 2; uint8_t _t5[7];
            uint8_t VIU_AL_ColorSet_R18 : 8; uint8_t VIU_AL_ColorSet_G18 : 8;
            uint8_t VIU_AL_ColorSet_B18 : 8; uint8_t VIU_AL_BriSet18 : 7;
            uint8_t VIU_AL_BriModSet18 : 1;

            uint8_t _t6 : 3; uint8_t VIU_AL_PreDishesArea19 : 5;
            uint8_t VIU_AL_PreDishesMod19 : 2; uint8_t _t7 : 2; uint8_t _t8[7];
            uint8_t VIU_AL_ColorSet_R19 : 8; uint8_t VIU_AL_ColorSet_G19 : 8;
            uint8_t VIU_AL_ColorSet_B19 : 8; uint8_t VIU_AL_BriSet19 : 7;
            uint8_t VIU_AL_BriModSet19 : 1;
        };
        uint8_t _0x1ed_data[64];
    } canfd_0x1ed;

    // ========================================================================
    // 0x1EE VIU_ALMusicFollowCtrl6 — 高音扬声器散光 (64 bytes)
    // ========================================================================
    union {
        struct {
            uint8_t VIU_AL_HLPreDishesMod : 2;
            uint8_t _u0 : 6;
            uint8_t VIU_AL_HLColorSet_R1 : 8; uint8_t VIU_AL_HLColorSet_G1 : 8;
            uint8_t VIU_AL_HLColorSet_B1 : 8;
            uint8_t VIU_AL_HLBriSet_1    : 7; uint8_t VIU_AL_HLBriModSet_1 : 1;

            uint8_t VIU_AL_HLColorSet_R2 : 8; uint8_t VIU_AL_HLColorSet_G2 : 8;
            uint8_t VIU_AL_HLColorSet_B2 : 8;
            uint8_t VIU_AL_HLBriSet_2    : 7; uint8_t VIU_AL_HLBriModSet_2 : 1;

            uint8_t VIU_AL_HLColorSet_R3 : 8; uint8_t VIU_AL_HLColorSet_G3 : 8;
            uint8_t VIU_AL_HLColorSet_B3 : 8;
            uint8_t VIU_AL_HLBriSet_3    : 7; uint8_t VIU_AL_HLBriModSet_3 : 1;

            uint8_t VIU_AL_HLColorSet_R4 : 8; uint8_t VIU_AL_HLColorSet_G4 : 8;
            uint8_t VIU_AL_HLColorSet_B4 : 8;
            uint8_t VIU_AL_HLBriSet_4    : 7; uint8_t VIU_AL_HLBriModSet_4 : 1;

            uint8_t VIU_AL_HLColorSet_R5 : 8; uint8_t VIU_AL_HLColorSet_G5 : 8;
            uint8_t VIU_AL_HLColorSet_B5 : 8;
            uint8_t VIU_AL_HLBriSet_5    : 7; uint8_t VIU_AL_HLBriModSet_5 : 1;

            uint8_t VIU_AL_HLColorSet_R6 : 8; uint8_t VIU_AL_HLColorSet_G6 : 8;
            uint8_t VIU_AL_HLColorSet_B6 : 8;
            uint8_t VIU_AL_HLBriSet_6    : 7; uint8_t VIU_AL_HLBriModSet_6 : 1;
        };
        uint8_t _0x1ee_data[64];
    } canfd_0x1ee;

} CanFrame;

// 全局接收帧 — 中断/轮询直接更新，应用层直接读
extern CanFrame g_rx_frame;

#endif
