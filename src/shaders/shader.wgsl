struct Uniforms {
    mvp : mat4x4f,
};

@group(0) @binding(0) var<uniform> uniforms : Uniforms;

struct VertOut {
    @builtin(position) pos : vec4f,
    @location(0)       col : vec3f,
};

@vertex fn vertexMain(@location(0) pos: vec3f) -> VertOut {
    return VertOut(uniforms.mvp * vec4f(pos, 1.0), pos + vec3f(0.5));
}

@fragment fn fragmentMain(@location(0) col: vec3f) -> @location(0) vec4f {
    return vec4f(col, 1.0);
}