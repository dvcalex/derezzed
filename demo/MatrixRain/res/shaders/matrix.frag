#version 460 core

in vec2 v_uv;

uniform float u_time;
uniform vec2 u_resolution;

layout(location = 0) out vec4 color;

float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float trailBrightness(float cellY, float headY, float trailLen)
{
    // distance from head to any given cell
    float dist = headY - cellY;
    if (dist < 0.0) dist += 1.0; // wrap-around
    if (dist < 0.0 || dist > trailLen) {
        return 0.0;
    }
    return exp(-dist / trailLen); // exponential decay for some shaping and smoothness
}

void main()
{
    // uv with flipped Y (0,1)<>(1,0)
    vec2 uv = vec2(v_uv.x, 1.0 - v_uv.y);

    // get cell indices
    float numCols = 40.0;
    float numRows = numCols * (u_resolution.y / u_resolution.x);
    vec2 cellIndex = floor(uv * vec2(numCols, numRows)); // which cell (integer)
    vec2 cellUV = fract(uv * vec2(numCols, numRows)); // position within cell

    float c = cellIndex.x;
    float r = cellIndex.y;

    // per-column random parameters
    float speed = 0.4 + hash(c * 73.1) * 0.8;
    float phase = hash(c * 127.1);
    float trailLen = 0.3 + hash(c * 311.7) * 0.4;

    // matrix rain "head" position wraps around 0->1->0...
    float headY = fract(u_time * speed + phase);

    // normalize row position 0<>1
    float cellY = r / numRows;

    // trail brightness
    float brightness = trailBrightness(cellY, headY, trailLen);

    // random changes per cell for a flickering effect
    float random = hash2(vec2(c, r + floor(u_time * 12.0)));
    brightness *= 0.6 + random * 0.4;

    float headDist = abs(cellY - headY);
    float headGlow = smoothstep(0.03, 0.0, headDist);
    vec3 green = vec3(0.1, brightness, brightness * 0.3);
    vec3 white = vec3(1.0);
    vec3 col = mix(green, white, headGlow * brightness);
    color = vec4(col, 1.0);
}
