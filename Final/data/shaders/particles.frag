#version 330

//out color
out vec4 fragColor;

uniform sampler2D u_diffuse_map;

void main(){
	//fragColor =  texture(u_diffuse_map, gl_PointCoord);
    vec4 tex = texture(u_diffuse_map, gl_PointCoord);
	//para poner transparencia, pero me falta la trasparencia
	fragColor = vec4(tex.xyz, tex.w );
}
