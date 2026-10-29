#pragma once

#include <algorithm>

namespace
{
    struct FrameStats
    {
        float fps = 0.f;
        float frameMs = 0.f;
        float updateMs = 0.f;
        float drawMs = 0.f;
        float worstFrameMs = 0.f;

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
                const float inv = 1.f / windowTime;

                fps = frames * inv;
                frameMs = windowTime * 1000.f * inv;
                updateMs = updateTime * 1000.f * inv;
                drawMs = drawTime * 1000.f * inv;
                worstFrameMs = windowWorst;

                windowTime = updateTime = drawTime = windowWorst = 0.f;
                frames = 0;
            }
        }
    };
}