#pragma once

namespace dingosdk::vr::detail {
constexpr char shader_source[] = R"(
Texture2D color : register(t0);
Texture2D<float> depth : register(t1);
SamplerState linear_clamp : register(s0);
cbuffer Constants : register(b0) { uint mode; float side; float scale; float max_shift; float vignette; };
struct Vertex { float4 position : SV_Position; float2 uv : TEXCOORD0; };
Vertex vs_main(uint id : SV_VertexID) {
    Vertex v;
    float2 t = float2((id << 1) & 2, id & 2);
    v.position = float4(t * float2(2, -2) + float2(-1, 1), 0, 1);
    v.uv = t;
    return v;
}
float3 srgb_to_linear(float3 c) {
    float3 low = c / 12.92;
    float3 high = pow((c + 0.055) / 1.055, 2.4);
    return lerp(high, low, step(c, 0.04045));
}
float shift_at(float2 uv, float2 size) {
    int2 p = int2(clamp(uv, 0, 0.99999) * size);
    return depth.Load(int3(p, 0)) * scale;
}
// Marches from the largest shift down: the first source point whose own shift
// reaches the ray (f >= 0) is the nearest surface along it. The crossing is then
// interpolated between the two steps for a sub-pixel, stable edge. At t = 0 the
// test always passes. A jump from clearly behind to clearly in front is a depth
// edge, not a surface: the pixel is a hole the centre view never saw. Stretch
// fills it with the near edge; background fill (mode bit 8) with the farthest
// surface immediately before the silhouette.
// Widens the near depth by a few pixels (max filter, reversed Z: larger = nearer), once
// per kept frame: anti-aliased and fringed edge pixels then move with their object
// instead of tearing off as lines between the eyes.
// `mode` holds the radius in pixels here (0-3); 0 copies the depth as it is.
float ps_dilate(Vertex v) : SV_Target {
    float2 size;
    depth.GetDimensions(size.x, size.y);
    int2 p = int2(v.position.xy);
    int radius = int(mode);
    int rows = min(radius, 1);
    float nearest = 0;
    [loop] for (int dy = -rows; dy <= rows; ++dy)
        [loop] for (int dx = -radius; dx <= radius; ++dx)
            nearest = max(nearest, depth.Load(int3(clamp(p + int2(dx, dy), int2(0, 0), int2(size) - 1), 0)));
    return nearest;
}
static const int steps = 48;
// Comfort vignette: darkens towards the edges by `amount` (0 none, 1 a narrow window). The
// image is rendered wider than the lenses show, so it starts well inside the image edge.
float3 comfort(float3 c, float2 uv, float amount) {
    if (amount <= 0) return c;
    float r = length((uv - 0.5) * 2);
    float inner = lerp(0.95, 0.15, amount);
    return c * (1 - smoothstep(inner, inner + 0.35, r));
}
float4 ps_main(Vertex v) : SV_Target {
    float x = v.uv.x;
    uint decode = mode & 0xff;
    bool background_fill = (mode & 0x100) != 0;
    if (scale > 0 && max_shift > 0) {
        float2 size;
        depth.GetDimensions(size.x, size.y);
        float step_size = max_shift / steps;
        float previous_t = max_shift, previous_f = shift_at(float2(v.uv.x - side * max_shift, v.uv.y), size) - max_shift;
        [loop] for (int i = steps - 1; i >= 0; --i) {
            float t = step_size * i;
            float sx = v.uv.x - side * t;
            float s = shift_at(float2(sx, v.uv.y), size);
            float f = s - t;
            if (f >= 0) {
                bool hole = previous_f < -2 * step_size && f > 2 * step_size;
                float crossing = previous_f < 0 ? t + (previous_t - t) * f / max(f - previous_f, 1e-6) : t;
                // Use the background immediately before this silhouette, rather
                // than the farthest depth anywhere along the search. The latter
                // imports unrelated distant colours into thin gaps by the board.
                // Snap to its texel centre so linear sampling cannot blend it
                // back across the foreground edge.
                float background_x = (floor((v.uv.x - side * previous_t) * size.x) + 0.5) / size.x;
                x = hole && background_fill ? background_x : v.uv.x - side * crossing;
                break;
            }
            previous_t = t;
            previous_f = f;
        }
    }
    float3 c = saturate(color.SampleLevel(linear_clamp, float2(x, v.uv.y), 0).rgb);
    if (decode == 0) c = srgb_to_linear(c);
    return float4(comfort(c, v.uv, vignette), 1);
}
)";
}
