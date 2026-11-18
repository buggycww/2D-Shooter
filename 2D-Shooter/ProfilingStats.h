#pragma once

#include <algorithm>

namespace
{
    struct FrameStats
    {
        // PUBLISHED VALUES (read these to draw the HUD) ============================
        float fps = 0.f; 
        float frameMs = 0.f; // avg full frame time  (== 1000/fps)
        float updateMs = 0.f; // avg CPU time in update
        float drawMs = 0.f; // avg CPU time submitting draws
        float worstFrameMs = 0.f; // worst single frame in window

        // ACCUMULATTORS FOR CURRENT WINDOW (seconds) ===============================
        float windowTime = 0.f;
        float updateTime = 0.f;
        float drawTime = 0.f;
        float windowWorst = 0.f;
        int frames = 0;

        void Add(float frameSeconds, float updateSeconds, float drawSeconds)
        {
            windowTime += frameSeconds;
            updateTime += updateSeconds;
            drawTime += drawSeconds;
            windowWorst = std::max(windowWorst, frameSeconds * 1000.f);
            ++frames;

            // update  displayed numbers a few times per second instead of every frame.
            if (windowTime >= 0.25f)
            {
                const double invFrames = 1.0 / frames;
                const double invTime = 1.0 / windowTime;

                fps = frames * invTime;
                frameMs = windowTime * 1000.f * invFrames;
                updateMs = updateTime * 1000.f * invFrames;
                drawMs = drawTime * 1000.f * invFrames;
                worstFrameMs = windowWorst;

                windowTime = updateTime = drawTime = windowWorst = 0.f;
                frames = 0;
            }
        }
    };
}