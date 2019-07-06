//
//  Game.cpp
//
//  Copyright � 2018 Alun Evans. All rights reserved.
//

#include "Game.h"
#include "Shader.h"
#include "extern.h"
#include "Parsers.h"
#include "script.h"


Game::Game() {

}

//Nothing here yet
void Game::init(int w, int h) {

	window_width_ = w; window_height_ = h;
	//******* INIT SYSTEMS *******

	//init systems except debug, which needs info about scene
	control_system_.init();
	graphics_system_.init(window_width_, window_height_, "data/assets/");
	debug_system_.init(&graphics_system_);
    script_system_.init(&control_system_);
	gui_system_.init(window_width_, window_height_);
    animation_system_.init();
	particle_system_.init();

	/******** SHADERS **********/

	graphics_system_.screen_background_color = lm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	Shader* blend_shader = graphics_system_.loadShader("data/shaders/phong_blend.vert", "data/shaders/phong.frag");
	Shader* phong_shader = graphics_system_.loadShader("data/shaders/phong.vert", "data/shaders/phong.frag");
	Shader* cubemap_shader = graphics_system_.loadShader("data/shaders/cubemap.vert", "data/shaders/cubemap.frag");
	Shader* reflection_shader = graphics_system_.loadShader("data/shaders/reflection.vert", "data/shaders/reflection.frag");
	Shader* terrain_shader = graphics_system_.loadShader("data/shaders/phong.vert", "data/shaders/terrain.frag");
	Shader* animation_shader = graphics_system_.loadShader("data/shaders/phong_anim.vert", "data/shaders/phong.frag");

	

	//BLEND FACE--------------------------------------------------------------------------------------------------------
	//basic blue material

	Parsers::parseMTL("data/assets/toon/", "toon_base.mtl", graphics_system_.getMaterials(), blend_shader->program);

	int toon_ent = ECS.createEntity("toon");
	Mesh& toon_mesh = ECS.createComponentForEntity<Mesh>(toon_ent);
	//create multi-material-set geometry
	toon_mesh.geometry = Parsers::parseOBJ_multi("data/assets/toon/toon_base.obj",
		graphics_system_.getGeometries(),
		graphics_system_.getMaterials());

	//get reference to geom object
	Geometry& toon_geom = graphics_system_.getGeometries()[toon_mesh.geometry];

	ECS.getComponentFromEntity<Transform>(toon_ent).translate(lm::vec3(100, 0, -100));


	//add blend shapes here 
	std::vector<float> vertices, normals, uvs;
	std::vector<unsigned int> indices;

	Parsers::parseOBJ("data/assets/toon/toon_happy.obj", vertices, normals, uvs, indices);

	toon_geom.addBlendShape(vertices);

	BlendShapes& blend_comp = ECS.createComponentForEntity<BlendShapes>(toon_ent);
	blend_comp.addShape("happy");
	blend_comp.blend_weights[0] = 0.0;

	std::vector<float> vertices2, normals2, uvs2;
	std::vector<unsigned int> indices2;

	Parsers::parseOBJ("data/assets/toon/toon_angry.obj", vertices2, normals2, uvs2, indices2);

	toon_geom.addBlendShape(vertices2);

	blend_comp.addShape("angry");
	blend_comp.blend_weights[1] = 0.0;

	

	//END FACES BLENDING-----------------------------------------------------------------------------


	/******** MATERIALS **********/
	//basic blue material
	int mat_blue_check_index = graphics_system_.createMaterial();
	Material& mat_blue_check = graphics_system_.getMaterial(mat_blue_check_index);
	mat_blue_check.shader_id = phong_shader->program;
	mat_blue_check.name = "Blue";
	mat_blue_check.diffuse_map = Parsers::parseTexture("data/assets/block_blue.tga");
	mat_blue_check.specular = lm::vec3(0, 0, 0);
	
	std::vector<std::string> cube_faces{
		"data/assets/skybox/right.tga","data/assets/skybox/left.tga",
		"data/assets/skybox/top.tga","data/assets/skybox/bottom.tga",
		"data/assets/skybox/front.tga", "data/assets/skybox/back.tga" };

	GLuint cubemap_texture = Parsers::parseCubemap(cube_faces);

	int mat_reflection_index = graphics_system_.createMaterial();
	Material& ref_mat = graphics_system_.getMaterial(mat_reflection_index);
	ref_mat.shader_id = reflection_shader->program;
	ref_mat.cube_map = cubemap_texture;
	ref_mat.name = "Reflection Ball";
	//light
	int ent_light = ECS.createEntity("light");
	ECS.getComponentFromEntity<Transform>(ent_light).translate(0, 17, 17);
	Light& light_comp = ECS.createComponentForEntity<Light>(ent_light);
	light_comp.direction = lm::vec3(0.0f, -1.0f, -0.7f);
	light_comp.color = lm::vec3(1.0f, 1.0f, 1.0f);
	light_comp.type = LightTypeDirectional;
	light_comp.position = lm::vec3(0, 1, 0);
	light_comp.forward = light_comp.direction.normalize();
	light_comp.setOrthographic(-10, 10, -10, 10, 8, 30);
	light_comp.cast_shadow = 1;
	light_comp.update();

	//light 1
	int ent_light1 = ECS.createEntity("spot");
	ECS.getComponentFromEntity<Transform>(ent_light1).translate(0, 17, 17);
	Light& light_comp1 = ECS.createComponentForEntity<Light>(ent_light1);
	light_comp1.direction = lm::vec3(0.0f, -1.0f, -0.7f);
	light_comp1.color = lm::vec3(1.0f, 0.0f, 1.0f);
	light_comp1.type = LightTypeSpot;
	light_comp1.position = lm::vec3(0, 1, 0);
	light_comp1.forward = light_comp1.direction.normalize();
	light_comp1.setOrthographic(-10, 10, -10, 10, 8, 30);
	light_comp1.cast_shadow = 1;
	light_comp1.update();

	//light 1
	int ent_light2 = ECS.createEntity("point");
	ECS.getComponentFromEntity<Transform>(ent_light2).translate(0, 3, 10);
	Light& light_comp2 = ECS.createComponentForEntity<Light>(ent_light2);
	light_comp2.direction = lm::vec3(0.0f, -1.0f, -0.7f);
	light_comp2.color = lm::vec3(0.0f, 1.0f, 0.0f);
	light_comp2.type = LightTypePoint;
	light_comp2.position = lm::vec3(0, 1, 0);
	light_comp2.forward = light_comp1.direction.normalize();
	light_comp2.setOrthographic(-10, 10, -10, 10, 8, 30);
	light_comp2.cast_shadow = 1;
	light_comp2.update();

	int sphere_geom = graphics_system_.createGeometryFromFile("data/assets/sphere.obj");
	int geom_floor = graphics_system_.createGeometryFromFile("data/assets/floor_40x40.obj");
	//floor
	int floor_entity = ECS.createEntity("floor");
	ECS.getComponentFromEntity<Transform>(floor_entity).translate(0.0f, 0.0f, 0.0f);
	Mesh& floor_mesh = ECS.createComponentForEntity<Mesh>(floor_entity);
	floor_mesh.geometry = geom_floor;
	floor_mesh.material = mat_blue_check_index;

	//phong sphere
	int sphere_entity = ECS.createEntity("phong_sphere");
	Transform& st = ECS.getComponentFromEntity<Transform>(sphere_entity);
	st.translate(1.0f, 6.0f, 5.0f);

	
	Mesh& sphere_mesh = ECS.createComponentForEntity<Mesh>(sphere_entity);
	sphere_mesh.geometry = sphere_geom;
	sphere_mesh.material = mat_reflection_index;
	
	//terrain material and noise map
	//noise map data - must be cleaned up
	ImageData noise_image_data;
	float terrain_height = 20.0f;

	int mat_terrain_index = graphics_system_.createMaterial();
	Material& mat_terrain = graphics_system_.getMaterial(mat_terrain_index);
	mat_terrain.shader_id = terrain_shader->program;
	mat_terrain.specular = lm::vec3(0, 0, 0);
	mat_terrain.name = "Terrain";
	mat_terrain.diffuse_map = Parsers::parseTexture("data/assets/terrain/grass01.tga");
	mat_terrain.diffuse_map_2 = Parsers::parseTexture("data/assets/terrain/cliffs.tga");
	mat_terrain.normal_map = Parsers::parseTexture("data/assets/terrain/grass01_n.tga");
	//read texture, pass optional variables to get pointer to pixel data
	mat_terrain.noise_map = Parsers::parseTexture("data/assets/terrain/heightmap1.tga",
		&noise_image_data,
		true);
	graphics_system_.height = terrain_height;
	mat_terrain.height = terrain_height;
	mat_terrain.uv_scale = lm::vec2(100, 100);
	graphics_system_.uv = mat_terrain.uv_scale;

	

	/******** GEOMETRIES & ENVIRONMENT **********/

	//environment
	int cubemap_geom = graphics_system_.createGeometryFromFile("data/assets/cubemap.obj");
	
	graphics_system_.setEnvironment(Parsers::parseCubemap(cube_faces),
		cubemap_geom, cubemap_shader->program);

	//terrain
	//create terrain geometry - this function is a wrapper for Geometry::createTerrain
	int terrain_geometry = graphics_system_.createTerrainGeometry(500,
		0.4f,
		terrain_height,
		noise_image_data);


	//delete noise_image data other we have a memory leak
	delete noise_image_data.data;


	//******** ENTITIES  **********/
	

	//terrain
	int terrain_entity = ECS.createEntity("Terrain");
	Mesh& terrain_mesh = ECS.createComponentForEntity<Mesh>(terrain_entity);
	terrain_mesh.geometry = terrain_geometry;
	terrain_mesh.material = mat_terrain_index;
	terrain_mesh.render_mode = RenderModeForward;

	

	ECS.getComponentFromEntity<Transform>(terrain_entity).translate(lm::vec3(0, -10, 0));


	//create scene contents here
	

	int ball_ent = ECS.createEntity("ball");
	Mesh& ball_mesh = ECS.createComponentForEntity<Mesh>(ball_ent);
	ECS.createComponentForEntity<Transform>(ball_ent);
	ball_mesh.geometry = graphics_system_.createGeometryFromFile("data/assets/ball.obj");
	ball_mesh.material = graphics_system_.createMaterial();
	graphics_system_.getMaterial(ball_mesh.material).name = "Anim";
	graphics_system_.getMaterial(ball_mesh.material).shader_id = phong_shader->program;
	

	Parsers::parseAnimation("data/assets/bounce.anim");

	

	//skinned animation
	Parsers::parseCollada("data/assets/Punching.dae", animation_shader, graphics_system_);
    
	//ECS.createComponentForEntity<Light>(ECS.createEntity("Directional Light"));

	//create camera
    graphics_system_.screen_background_color = lm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    createFreeCamera_(13.614, 16, 32, -0.466, -0.67, -0.579);


	//Scripts------------------------------------------
	int scripts_ent = ECS.createEntity("scripts");
	script* console_script = new script(scripts_ent);
	debug_system_.consoleScript = console_script;
	script_system_.registerScript(console_script);

	//END Scripts--------------------------------------
	
	//Particles----------------------------------------
	int particles_ent = ECS.createEntity("Particles");
	ECS.createComponentForEntity<Transform>(particles_ent);
	ECS.createComponentForEntity<ParticleEmitter>(particles_ent);

	ParticleEmitter& emitter = ECS.getComponentFromEntity<ParticleEmitter>(particles_ent);
	emitter.max_life = 3000;
	emitter.name = "Sistema 1";
	emitter.min_life = 1000;
	emitter.particle_size = 1;
	emitter.num_particles = 1000;
	
	particle_system_.create();

	//END Particles--------------------------------------------------

    //******* LATE INIT AFTER LOADING RESOURCES *******//
    graphics_system_.lateInit();
    script_system_.lateInit();
    animation_system_.lateInit();
    debug_system_.lateInit();

	debug_system_.setActive(true);

}

//update each system in turn
void Game::update(float dt) {

	if (ECS.getAllComponents<Camera>().size() == 0) {print("There is no camera set!"); return;}

	//update input
	control_system_.update(dt);

	//collision
	collision_system_.update(dt);

    //animation
    animation_system_.update(dt);
    
	//scripts
	script_system_.update(dt);

	//render
	graphics_system_.update(dt);
    
    //particles
	particle_system_.update();
    
	//gui
	gui_system_.update(dt);

	//debug
	debug_system_.update(dt);

   
}
//update game viewports
void Game::update_viewports(int window_width, int window_height) {
	window_width_ = window_width;
	window_height_ = window_height;

	auto& cameras = ECS.getAllComponents<Camera>();
	for (auto& cam : cameras) {
		cam.setPerspective(60.0f*DEG2RAD, (float)window_width_ / (float) window_height_, 0.01f, 10000.0f);
	}

	graphics_system_.updateMainViewport(window_width_, window_height_);
}

Material& Game::createMaterial(GLuint shader_program) {
    int mat_index = graphics_system_.createMaterial();
    Material& ref_mat = graphics_system_.getMaterial(mat_index);
    ref_mat.shader_id = shader_program;
    return ref_mat;
}

int Game::createFreeCamera_(float px, float py, float pz, float fx, float fy, float fz) {
	int ent_player = ECS.createEntity("PlayerFree");
	Camera& player_cam = ECS.createComponentForEntity<Camera>(ent_player);
    lm::vec3 the_position(px, py, px);
	ECS.getComponentFromEntity<Transform>(ent_player).translate(the_position);
	player_cam.position = the_position;
	player_cam.forward = lm::vec3(fx, fy, fz);
	player_cam.setPerspective(60.0f*DEG2RAD, (float)window_width_/(float)window_height_, 0.1f, 1000.0f);

	ECS.main_camera = ECS.getComponentID<Camera>(ent_player);

	control_system_.control_type = ControlTypeFree;

	return ent_player;
}

int Game::createPlayer_(float aspect, ControlSystem& sys) {
	int ent_player = ECS.createEntity("PlayerFPS");
	Camera& player_cam = ECS.createComponentForEntity<Camera>(ent_player);
	lm::vec3 the_position(0.0f, 3.0f, 5.0f);
	ECS.getComponentFromEntity<Transform>(ent_player).translate(the_position);
	player_cam.position = the_position;
	player_cam.forward = lm::vec3(0.0f, 0.0f, -1.0f);
	player_cam.setPerspective(60.0f*DEG2RAD, aspect, 0.01f, 10000.0f);

	//FPS colliders 
	//each collider ray entity is parented to the playerFPS entity
	int ent_down_ray = ECS.createEntity("Down Ray");
	Transform& down_ray_trans = ECS.getComponentFromEntity<Transform>(ent_down_ray);
	down_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
	Collider& down_ray_collider = ECS.createComponentForEntity<Collider>(ent_down_ray);
	down_ray_collider.collider_type = ColliderTypeRay;
	down_ray_collider.direction = lm::vec3(0.0, -1.0, 0.0);
	down_ray_collider.max_distance = 100.0f;

	int ent_left_ray = ECS.createEntity("Left Ray");
	Transform& left_ray_trans = ECS.getComponentFromEntity<Transform>(ent_left_ray);
	left_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
	Collider& left_ray_collider = ECS.createComponentForEntity<Collider>(ent_left_ray);
	left_ray_collider.collider_type = ColliderTypeRay;
	left_ray_collider.direction = lm::vec3(-1.0, 0.0, 0.0);
	left_ray_collider.max_distance = 1.0f;

	int ent_right_ray = ECS.createEntity("Right Ray");
	Transform& right_ray_trans = ECS.getComponentFromEntity<Transform>(ent_right_ray);
	right_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
	Collider& right_ray_collider = ECS.createComponentForEntity<Collider>(ent_right_ray);
	right_ray_collider.collider_type = ColliderTypeRay;
	right_ray_collider.direction = lm::vec3(1.0, 0.0, 0.0);
	right_ray_collider.max_distance = 1.0f;

	int ent_forward_ray = ECS.createEntity("Forward Ray");
	Transform& forward_ray_trans = ECS.getComponentFromEntity<Transform>(ent_forward_ray);
	forward_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
	Collider& forward_ray_collider = ECS.createComponentForEntity<Collider>(ent_forward_ray);
	forward_ray_collider.collider_type = ColliderTypeRay;
	forward_ray_collider.direction = lm::vec3(0.0, 0.0, -1.0);
	forward_ray_collider.max_distance = 1.0f;

	int ent_back_ray = ECS.createEntity("Back Ray");
	Transform& back_ray_trans = ECS.getComponentFromEntity<Transform>(ent_back_ray);
	back_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
	Collider& back_ray_collider = ECS.createComponentForEntity<Collider>(ent_back_ray);
	back_ray_collider.collider_type = ColliderTypeRay;
	back_ray_collider.direction = lm::vec3(0.0, 0.0, 1.0);
	back_ray_collider.max_distance = 1.0f;

	//the control system stores the FPS colliders 
	sys.FPS_collider_down = ECS.getComponentID<Collider>(ent_down_ray);
	sys.FPS_collider_left = ECS.getComponentID<Collider>(ent_left_ray);
	sys.FPS_collider_right = ECS.getComponentID<Collider>(ent_right_ray);
	sys.FPS_collider_forward = ECS.getComponentID<Collider>(ent_forward_ray);
	sys.FPS_collider_back = ECS.getComponentID<Collider>(ent_back_ray);

	ECS.main_camera = ECS.getComponentID<Camera>(ent_player);

	sys.control_type = ControlTypeFPS;

	return ent_player;
}

