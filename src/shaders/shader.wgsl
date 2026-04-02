struct VertOut {
    @builtin(position) pos : vec4f,
    @location(0)       col : vec3f,
};

@vertex fn vertexMain(@location(0) pos: vec3f) -> VertOut {
    // Rotate around Y (30°)
    let ay = 0.5236f;
    let cy = cos(ay); let sy = sin(ay);
    var p = vec3f(cy * pos.x + sy * pos.z, pos.y, -sy * pos.x + cy * pos.z);

    // Rotate around X (20°) and push back from camera
    let ax = 0.3491f;
    let cx = cos(ax); let sx = sin(ax);
    p = vec3f(p.x, cx * p.y - sx * p.z, sx * p.y + cx * p.z + 3.0f);

    // Perspective projection  (near=0.1, far=10, aspect=4/3, fov~60°)
    let f = 1.3f;
    let clipPos = vec4f(p.x * f * 0.75f, p.y * f,
                        p.z * (10.0f / 9.9f) - (1.0f / 9.9f), p.z);

    // Use the original local-space position as RGB color (shift to 0..1 range)
    return VertOut(clipPos, pos + vec3f(0.5f));
}

@fragment fn fragmentMain(@location(0) col: vec3f) -> @location(0) vec4f {
    return vec4f(col, 1.0);
}

