@binding(0) @group(0) var<uniform> in_position : vec2f;
@binding(0) @group(1) var<uniform> in_color : vec3f;

struct VertexOutput {
  @builtin(position) Position : vec4f,
  @location(0) fragColor: vec3f,
}

@vertex
fn vtx_main(@builtin(vertex_index) vertex_index : u32) -> VertexOutput {
  var output : VertexOutput;
  output.Position = vec4f(in_position, 0, 1);
  output.fragColor = in_color;
  return output;
}

@fragment
fn frag_main(@location(0) fragColor: vec3f) -> @location(0) vec4f {
  return vec4f(fragColor, 1);
}
