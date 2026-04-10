struct Uniforms {
    mvp : mat4x4f,
};

@group(0) @binding(0) var<uniform> uniforms    : Uniforms;
@group(0) @binding(1) var          tex         : texture_2d<f32>;
@group(0) @binding(2) var          texSampler  : sampler;

struct VertOut {
    @builtin(position) pos : vec4f,
    @location(0)       uv  : vec2f,
};

@vertex fn vertexMain(
    @location(0) pos: vec3f, 
    @location(1) uv: vec2f
) -> VertOut {
    return VertOut(uniforms.mvp * vec4f(pos, 1.0), uv);
}

@fragment fn fragmentMain(@location(0) uv: vec2f) -> @location(0) vec4f {
    return textureSample(tex, texSampler, uv);
}