#pragma once

#include <math.h>
#include <vector>
#include <map>
#include <string>

const float M_PI = 3.14159265359f;
struct RSColor {
	float r = 1.0f;
	float g = 0.0f;
	float b = 0.0f;

	RSColor() = default;

    RSColor(int R, int G, int B) : r(R / 255.0f), g(G / 255.0f), b(B / 255.0f) {}

    RSColor(float R, float G, float B) : r(R), g(G), b(B) {}

    void setH(float H) {
        const float rad = H * M_PI / 180.0f;
        const float U = cosf(rad);
        const float W = sinf(rad);

        r = 0.299f + 0.701f * U + 0.168f * W;
        g = 0.299f - 0.299f * U - 0.328f * W;
        b = 0.299f - 0.300f * U + 1.250f * W;
    }
};

using ColorMap = std::map<std::string, RSColor>;
using ColorList = std::vector<RSColor>;