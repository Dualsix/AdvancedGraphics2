#version 330

layout(location = 0) in vec3 a_vertex;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec4 a_vertex_weights;
layout(location = 4) in vec4 a_vertex_jointids;


const int MAX_JOINTS = 96;
uniform mat4 u_joint_pos_matrices[MAX_JOINTS];
uniform mat4 u_joint_bind_matrices[MAX_JOINTS];

uniform mat4 u_skin_bind_matrix;

uniform mat4 u_mvp;
uniform mat4 u_vp;
uniform mat4 u_model;
uniform mat4 u_normal_matrix;
uniform vec3 u_cam_pos;

out vec2 v_uv;
out vec3 v_normal;
out vec3 v_vertex_world_pos;
out vec3 v_cam_dir;

void main(){
    
    //uvs
    v_uv = a_uv;
    
    //calculate direction to camera in world space
    v_cam_dir = u_cam_pos - v_vertex_world_pos;
    
    //unweighted position of vertex
    vec4 vertex4 = vec4(a_vertex, 1.0);
    
    //premultiply by the skin bind matrix
    vec4 vertex4_bsm = u_skin_bind_matrix * vertex4;
    //normals are the mat3 (remove translation component)
    vec3 normal3_bsm = mat3(u_skin_bind_matrix) * a_normal;
    
    //variables to store final position and normal
    vec4 final_vert = vec4(0.0, 0.0, 0.0, 0.0);
    vec3 final_normal = vec3(0.0, 0.0, 0.0);
    
    //for all potential bones
    for (int i = 0; i < 4; i ++){
        //joint id (in chain) of joint 'i'
        int j_id = int(a_vertex_jointids[i]);

        // vertex -> bind pose -> animation frame pose -> joint weight
        final_vert += a_vertex_weights[i] *
                        u_joint_pos_matrices[j_id] *
                        u_joint_bind_matrices[j_id] *
                        vertex4_bsm;
        
        //same for normals, using mat3s
        final_normal += a_vertex_weights[i] *
                        mat3(u_joint_pos_matrices[j_id]) *
                        mat3(u_joint_bind_matrices[j_id]) *
                        normal3_bsm;


    }
    
    //set the final position and normal
    v_vertex_world_pos = final_vert.xyz;
    v_normal = final_normal;
    
    //output vertex to NDC
    gl_Position = u_vp * final_vert;
}

