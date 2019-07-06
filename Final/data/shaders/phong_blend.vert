#version 330

layout(location = 0) in vec3 a_vertex;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_blend0;
layout(location = 4) in vec3 a_blend1;

uniform mat4 u_mvp;
uniform mat4 u_model;
uniform mat4 u_normal_matrix;
uniform vec3 u_cam_pos;

const int MAX_BLEND_SHAPES = 8;
uniform float [MAX_BLEND_SHAPES] u_blend_weights;

out vec2 v_uv;
out vec3 v_normal;
out vec3 v_vertex_world_pos;
out vec3 v_cam_dir;

void main(){

	vec3 offset0 = a_blend0 - a_vertex;
	vec3 offset1 = a_blend1 - a_vertex;

    vec3 mod_vertex = a_vertex + offset0 * u_blend_weights[0] + offset1 * u_blend_weights[1];
    
    
	v_uv = a_uv;
	//rotate normal & tangent
	v_normal = (u_normal_matrix * vec4(a_normal, 1.0)).xyz;
    
	//calculate world position of current vertex
	v_vertex_world_pos = (u_model * vec4(mod_vertex, 1.0)).xyz;

	//calculate direction to camera in world space
	v_cam_dir = u_cam_pos - v_vertex_world_pos;

	gl_Position = u_mvp * vec4(mod_vertex, 1.0);
}
