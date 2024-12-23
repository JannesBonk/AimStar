#pragma once
#define _USE_MATH_DEFINES
#define MAXV 10000e9
#include <math.h>
#include <thread>
#include <chrono>
#include "..\..\Game.h"
#include "..\..\Entity.h"
#include "..\..\MenuConfig.hpp"
#include <iostream>
#include "..\..\View.hpp"
#include "..\..\Features/RCS.h"
#include "..\..\Utils\XorStr.h"
#include "..\..\Entity.h"
#include "..\..\Features\Mouse.h"



namespace AimControl
{
    extern bool Rage;

    inline unsigned int HotKey = VK_LBUTTON;
    inline int AimBullet = 0;
    inline bool ScopeOnly = false;
    inline bool AutoShot = false;
    inline bool AimLock = false;
    inline bool IgnoreFlash = false;
    inline float AimFov = 5;
    inline float AimFovMin = 0.f;
    inline float Smooth = 2.0f;
    inline std::vector<int> HitboxList{ BONEINDEX::head };

    inline bool HasTarget = false;
    /*
    inline void SetHotKey(int Index)
    {
        HotKey = HotKeyList.at(Index);
    }
    */
    inline void switchToggle()
    {
        MenuConfig::AimAlways = !MenuConfig::AimAlways;
    }

    inline void AimBot(const CEntity& Local, Vec3 LocalPos, std::vector<Vec3>& AimPosList)
    {
        if (MenuConfig::ShowMenu)
            return;

        //int isFired;
        //ProcessMgr.ReadMemory(Local.Pawn.Address + Offset::Pawn.iShotsFired, isFired);
        //if (!isFired && !AimLock)
        // When players hold these weapons, don't aim
        std::vector<std::string> WeaponNames = {
        XorStr("smokegrenade"), XorStr("flashbang"), XorStr("hegrenade"), XorStr("molotov"), XorStr("decoy"), XorStr("incgrenade"),
        XorStr("ct_knife"), XorStr("t_knife"),XorStr("c4")
        };
        if (std::find(WeaponNames.begin(), WeaponNames.end(), Local.Pawn.WeaponName) != WeaponNames.end())
        {
            HasTarget = false;
            return;
        }

        if (Local.Pawn.ShotsFired <= AimBullet && !AimLock && AimBullet != 0)
        {
            HasTarget = false;
            return;
        }


        if (AimControl::ScopeOnly)
        {
            bool isScoped;
            ProcessMgr.ReadMemory<bool>(Local.Pawn.Address + Offset::C_CSPlayerPawn.m_bIsScoped, isScoped);
            if (!isScoped) {
                HasTarget = false;
                return;
            }
        }

        if (!IgnoreFlash && Local.Pawn.FlashDuration > 0.15f)
            return;

        if (MenuConfig::DRM) {//ONLY DRM
            gGame.SetViewAngle(rand() % 180, rand() % 89);
            gGame.SetForceJump(65537);
            gGame.SetForceCrouch(65537);
            gGame.SetForceMove(0,255);

            return;
        }
        int ListSize = AimPosList.size();
        float BestNorm = MAXV;

        float Yaw, Pitch;
        float Distance, Norm, Length;
        Vec2 Angles{ 0,0 };
        int ScreenCenterX = Gui.Window.Size.x / 2;
        int ScreenCenterY = Gui.Window.Size.y / 2;
        float TargetX = 0.f;
        float TargetY = 0.f;

        Vec2 ScreenPos;

        uintptr_t ClippingWeapon, WeaponData;
        bool IsAuto;
        ProcessMgr.ReadMemory(Local.Pawn.Address + Offset::C_CSPlayerPawnBase.m_pClippingWeapon, ClippingWeapon);
        ProcessMgr.ReadMemory(ClippingWeapon + Offset::WeaponBaseData.WeaponDataPTR, WeaponData);
        ProcessMgr.ReadMemory(WeaponData + Offset::WeaponBaseData.m_bIsFullAuto, IsAuto);

        for (int i = 0; i < ListSize; i++)
        {
            Vec3 OppPos;

            OppPos = AimPosList[i] - LocalPos;

            Distance = sqrt(pow(OppPos.x, 2) + pow(OppPos.y, 2));

            Length = OppPos.Length();

            // RCS by @Tairitsu
            if (MenuConfig::RCS && IsAuto)
            {

                RCS::UpdateAngles(Local, Angles);
                float rad = Angles.x * RCS::RCSScale.x / 360.f * M_PI;
                float si = sinf(rad);
                float co = cosf(rad);

                float z = OppPos.z * co + Distance * si;
                float d = (Distance * co - OppPos.z * si) / Distance;

                rad = -Angles.y * RCS::RCSScale.y / 360.f * M_PI;
                si = sinf(rad);
                co = cosf(rad);

                float x = (OppPos.x * co - OppPos.y * si) * d;
                float y = (OppPos.x * si + OppPos.y * co) * d;

                OppPos = Vec3{ x, y, z };

                AimPosList[i] = LocalPos + OppPos;
            }

            Yaw = atan2f(OppPos.y, OppPos.x) * 57.295779513 - Local.Pawn.ViewAngle.y;
            Pitch = -atan(OppPos.z / Distance) * 57.295779513 - Local.Pawn.ViewAngle.x;
            Norm = sqrt(pow(Yaw, 2) + pow(Pitch, 2));
                if (Norm < BestNorm)
                    BestNorm = Norm;
            gGame.View.WorldToScreen(Vec3(AimPosList[i]), ScreenPos);
        }

        if (Norm < AimFov && Norm > AimFovMin)
        {
            HasTarget = true;
            // Shake Fixed by @Sweely
            if (ScreenPos.x != ScreenCenterX)
            {
                TargetX = (ScreenPos.x > ScreenCenterX) ? -(ScreenCenterX - ScreenPos.x) : ScreenPos.x - ScreenCenterX;
                TargetX /= Smooth != 0.0f ? Smooth : 1.5f;
                TargetX = (TargetX + ScreenCenterX > ScreenCenterX * 2 || TargetX + ScreenCenterX < 0) ? 0 : TargetX;
            }

            if (ScreenPos.y != 0)
            {
                if (ScreenPos.y != ScreenCenterY)
                {
                    TargetY = (ScreenPos.y > ScreenCenterY) ? -(ScreenCenterY - ScreenPos.y) : ScreenPos.y - ScreenCenterY;
                    TargetY /= Smooth != 0.0f ? Smooth : 1.5f;
                    TargetY = (TargetY + ScreenCenterY > ScreenCenterY * 2 || TargetY + ScreenCenterY < 0) ? 0 : TargetY;
                }
            }

            if (!Smooth)
            {
                mouse_event(MOUSEEVENTF_MOVE, (DWORD)(TargetX), (DWORD)(TargetY), NULL, NULL);
                if (AutoShot)
                {
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                }
                return;
            }

            // Dynamic AimSmooth based on distance
            float DistanceRatio = Norm / AimFov; // Calculate the distance ratio
            float SpeedFactor = 1.0f + (1.0f - DistanceRatio); // Determine the speed factor based on the distance ratio
            TargetX /= (Smooth * SpeedFactor);
            TargetY /= (Smooth * SpeedFactor);
            // by Skarbor

            if (ScreenPos.x != ScreenCenterX)
            {
                TargetX = (ScreenPos.x > ScreenCenterX) ? -(ScreenCenterX - ScreenPos.x) : ScreenPos.x - ScreenCenterX;
                TargetX /= Smooth != 0.0f ? Smooth : 1.5f;
                TargetX = (TargetX + ScreenCenterX > ScreenCenterX * 2 || TargetX + ScreenCenterX < 0) ? 0 : TargetX;
            }

            if (ScreenPos.y != 0)
            {
                if (ScreenPos.y != ScreenCenterY)
                {
                    TargetY = (ScreenPos.y > ScreenCenterY) ? -(ScreenCenterY - ScreenPos.y) : ScreenPos.y - ScreenCenterY;
                    TargetY /= Smooth != 0.0f ? Smooth : 1.5f;
                    TargetY = (TargetY + ScreenCenterY > ScreenCenterY * 2 || TargetY + ScreenCenterY < 0) ? 0 : TargetY;
                }
            }

            mouse_event(MOUSEEVENTF_MOVE, TargetX, TargetY, NULL, NULL);
            if (AutoShot)
            {
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            }

            // Enable the Smooth setting temporarily
            int AimInterval = round(1000000.0f / MenuConfig::MaxFrameRate);
            std::this_thread::sleep_for(std::chrono::microseconds(AimInterval));
        }
        else
            HasTarget = false;
    }
}