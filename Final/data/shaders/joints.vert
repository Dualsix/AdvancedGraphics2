#version 330

layout(location = 0) in vec3 a_vertex;
const int MAX_JOINTS = 96;
uniform mat4 u_model[MAX_JOINTS];
uniform mat4 u_vp;


void main(){

    gl_PointSize = 5.0;
    
	gl_Position = u_vp * u_model[gl_VertexID] * vec4(a_vertex, 1.0);
}
