#version 330

layout(location = 0) in vec3 a_vertex;
layout(location = 1) in vec3 a_velocity;
layout(location = 2) in float a_age;
layout(location = 3) in float a_life;

out vec3 v_vertex;
out vec3 v_velocity;
out float v_age;
out float v_life;

uniform mat4 u_mvp;
uniform mat4 u_model;
uniform mat4 u_vp;
uniform float u_time;

uniform float u_height_near_plane;


float random(vec2 co){
	highp float a = 12.9898;
	highp float b = 78.233;
	highp float c = 43758.5453;
	highp float dt = dot(co.xy ,vec2(a,b));
	highp float sn = mod(dt,3.14);
	return fract(sin(sn) * c);
}

void main()
{
	
	float age = u_time - a_age;

	if(age > a_life){
		float r = random(vec2(gl_VertexID, u_time * 1000.0));
	
		//this makes a fountain spinningwith a bit of ranodm
		float angle = mod(u_time * 1, 6.283);
		vec3 ideal_dir = vec3(2.0 * cos(angle), 3.1415, 2.0 * sin(angle));
		vec3 randomized_dir = ideal_dir * vec3(r * 0.5 + 1.5); 

		//fountain
		v_vertex = vec3(0);
		v_velocity = randomized_dir;
		v_age = u_time;
		v_life = a_life;
	}
	else{
		//move particle
		v_velocity = a_velocity - vec3(0,0.005,0);
		v_vertex = a_vertex + v_velocity * 0.005;
		v_age = a_age;
		v_life = a_life;
	}
    
    gl_Position = u_vp * u_model * vec4(a_vertex, 1.0);
    
    gl_PointSize = (0.2 * u_height_near_plane) / gl_Position.w;
    
}
