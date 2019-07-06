#pragma once
#include "includes.h"
#include "Shader.h"
#include "Components.h"
struct AABB {
	lm::vec3 center;
	lm::vec3 half_width;
};

struct ImageData {
    GLubyte* data;
    int width;
    int height;
    int bytes_pp;
    bool getPixel(int x, int y, int pixel[3]) {
        
        if (x > width || y > height) return false;
        
        int pixel_location = width * bytes_pp * y + x * bytes_pp;
        
        pixel[0] = data[pixel_location];
        pixel[1] = data[pixel_location + 1];
        pixel[2] = data[pixel_location + 2];
        
        return true;
    }
};

struct Geometry {
    
    //constructors
    Geometry() { vao = 0; num_tris = 0; }
    Geometry(int a_vao, int a_tris) : vao(a_vao), num_tris(a_tris) {}
    Geometry(std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices);
    
    //core variables
	GLuint vao;
	GLuint num_tris;
	AABB aabb;
    
    //material sets
    void createMaterialSet(int tri_count, int material_id);
    std::vector<int> material_sets;
    std::vector<int> material_set_ids;
    
    //rendering
    void render();
    void render(int set);

	//geometry, arrays and AABB
	void createVertexArrays(std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices);
    int createPlaneGeometry();
	void setAABB(std::vector<GLfloat>& vertices);
    
    //terrain
    float max_terrain_height;
    int createTerrain(int resolution, float step, float max_height, ImageData& height_map);
    lm::vec3 calculateTerrainNormal(ImageData& height_map, int x, int y);
    
    //animation
    int addVertexWeights(std::vector<lm::vec4>& vertex_weights,
                         std::vector<lm::ivec4>& vertex_jointids);
    GLuint num_blend_shapes = 0;
    int addBlendShape(std::vector<float>& blend_offsets);
    std::vector<float> blend_weights;

};

struct Material {
	std::string name;
	int index = -1;
	int shader_id;
	lm::vec3 ambient;
	lm::vec3 diffuse;
	lm::vec3 specular;
	float specular_gloss;

	int diffuse_map;
    int diffuse_map_2;
    int diffuse_map_3;
	int cube_map;
    int normal_map;
    int specular_map;
    int transparency_map;
    float normal_factor;
    int noise_map;
    float height;
    
    lm::vec2 uv_scale;;

	Material() {
		name = "";
		ambient = lm::vec3(0.1f, 0.1f, 0.1f);
		diffuse = lm::vec3(1.0f, 1.0f, 1.0f);
		specular = lm::vec3(1.0f, 1.0f, 1.0f);
		diffuse_map = -1;
        diffuse_map_2 = -1;
        diffuse_map_3 = -1;
        noise_map = -1;
		cube_map = -1;
        normal_map = -1;
        specular_map = -1;
        transparency_map = -1;
        normal_factor = 1.0f;
		specular_gloss = 80.0f;
        uv_scale = lm::vec2(1.0f, 1.0f);
        height = 0.0f;
	}
};

struct Framebuffer {
	GLuint width, height;
	GLuint framebuffer = -1;
	GLuint num_color_attachments = 0;
	GLuint color_textures[10] = { 0,0,0,0,0,0,0,0,0,0 };
	void bindAndClear();
    void bindAndClear(lm::vec4 clear_color);
	void initColor(GLsizei width, GLsizei height);
	void initDepth(GLsizei width, GLsizei height);
    void initGbuffer(GLsizei width, GLsizei height);
};

