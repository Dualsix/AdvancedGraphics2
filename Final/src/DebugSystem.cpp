#include "DebugSystem.h"
#include "extern.h"
#include "Parsers.h"
#include "shaders_default.h"

DebugSystem::~DebugSystem() {
	delete grid_shader_;
	delete icon_shader_;
}

void DebugSystem::init(GraphicsSystem* gs) {
	graphics_system_ = gs;
}

void DebugSystem::lateInit() {
	//init booleans
	draw_grid_ = false;
	draw_icons_ = false;
	draw_frustra_ = false;
	draw_colliders_ = false;

	//compile debug shaders from strings in header file
	grid_shader_ = new Shader();
	grid_shader_->compileFromStrings(g_shader_line_vertex, g_shader_line_fragment);
	icon_shader_ = new Shader();
	icon_shader_->compileFromStrings(g_shader_icon_vertex, g_shader_icon_fragment);

	//create geometries
	createGrid_();
	createIcon_();
	createCube_();
	createRay_();

	//create texture for light icon
	icon_light_texture_ = Parsers::parseTexture("data/assets/icon_light.tga");
	icon_camera_texture_ = Parsers::parseTexture("data/assets/icon_camera.tga");

	//picking collider
	ent_picking_ray_ = ECS.createEntity("picking_ray");
	Collider& picking_ray = ECS.createComponentForEntity<Collider>(ent_picking_ray_);
	picking_ray.collider_type = ColliderTypeRay;
	picking_ray.direction = lm::vec3(0, 0, -1);
	picking_ray.max_distance = 0.001f;

    //bones
    joint_shader_ = new Shader("data/shaders/joints.vert", "data/shaders/joints.frag");
    createJointGeometry_();
    
	setActive(true);
}

//draws debug information or not
void DebugSystem::setActive(bool a) {
    active_ = a;
	draw_grid_ = a;
	draw_icons_ = a;
	draw_frustra_ = a;
	draw_colliders_ = a;
    draw_joints_ = a;
}

//called once per frame
void DebugSystem::update(float dt) {

    if (!active_) return;
    
    //line drawing first, use same shader
    if (draw_grid_ || draw_frustra_ || draw_colliders_) {
        
        //use line shader to draw all lines and boxes
        glUseProgram(grid_shader_->program);
        
        if (draw_grid_) {
            drawGrid_();
        }
        
        if (draw_frustra_) {
            drawFrusta_();
        }
        
        if (draw_colliders_) {
            drawColliders_();
        }
    }
    
    //icon drawing
    if (draw_icons_) {
        drawIcons_();
    }
    //joint drawing
    if (draw_joints_) {
        drawJoints_();
    }
    
    
    glBindVertexArray(0);
    
    //imGUI
    updateimGUI_(dt);
}

//Recursive function that creates joint index buffer which parent-child indices
void createJointIndexBuffer(Joint* current, std::vector<GLuint>& indices) {
    
    GLuint current_joint_index = current->index_in_chain;
    
    //only draw line if we have a parent
    if (current->parent) {
        //draw line from parent to current
        GLuint parent_index = current->parent->index_in_chain;
        indices.push_back(parent_index);
        indices.push_back(current_joint_index);
    }
    
    for (auto child : current->children){
        createJointIndexBuffer(child, indices);
    }
}

//create joint geometry
//the class member variables joints_vaos_ stores the VAO index for each
//joint chain. So we loop chains and create and array of vertices for
//each joint.
//EACH VERTEX POSITION IS (0,0,0). Why? Because we will pass joint positions
//to shader as uniforms
//However we do have to make an index buffer to draw lines, based on index of
//joints in tree
void DebugSystem::createJointGeometry_() {
    auto& skinnedmeshes = ECS.getAllComponents<SkinnedMesh>();
    for (auto& sm : skinnedmeshes) {
        if (!sm.root) continue; //if this mesh does not have a joint
        
        //count all joints in
        GLuint current_chain_count = sm.num_joints;
        std::vector<float> positions(current_chain_count * 3, 0);
        
        std::vector<GLuint> indices;
        createJointIndexBuffer(sm.root, indices);
        
        GLuint new_vao;
        glGenVertexArrays(1, &new_vao);
        glBindVertexArray(new_vao);
        //positions
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), &(positions[0]), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        //indices
        GLuint ibo;
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &(indices[0]), GL_STATIC_DRAW);
        
        //add to member variable storage
        joints_vaos_.push_back(new_vao);
    }
    
    
    //unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
}

//Recursive function which traverses joint tree depth first
//multiplies each joint model matrix by that of it's parent
//(to create a 'world' matrix for each joint), then copies
//that world matrix to an array of floats (all_matrices) which
//stores all of the joint matrices, one after another
//this float array is passed to the shader as a uniform
// Params:
//--------
// - current: the current joint, should pass root joint at first call
// - current_model: the current global model matrix (pass identity at first call
// - all_matrices: a vector which MUST BE of size 16 * num_joints
// - joint_count: integer passed by reference, used to track where we are in joint chain
void DebugSystem::getJointWorldMatrices_(Joint* current,
                                         lm::mat4 current_model,
                                         std::vector<float>& all_matrices,
                                         int& joint_count) {
    
    lm::mat4 joint_global_model = current_model * current->matrix;
    
    for (int i = 0; i < 16; i++)
        all_matrices[joint_count * 16 + i] = joint_global_model.m[i];
    
    for (auto& c : current->children) {
        joint_count++;
        getJointWorldMatrices_(c, joint_global_model, all_matrices, joint_count);
    }
}

//function that draws joints to screen
void DebugSystem::drawJoints_() {
    
    //joint shader
    glUseProgram(joint_shader_->program);
    
    Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
    auto& skinnedmeshes = ECS.getAllComponents<SkinnedMesh>();
    
    //skinned_meshes size must be same as joints_vaos size
    for (size_t i = 0; i < skinnedmeshes.size(); i++) {
        if (!skinnedmeshes[i].root) continue; //only draw if has joint chain
        
        //find uniform location (it is an array
        GLint u_model = glGetUniformLocation(joint_shader_->program, "u_model");
        
        //create float vector and fill it with mvps for each joint
        std::vector<float> all_matrices(skinnedmeshes[i].num_joints * 16, 0);
        int joint_counter = 0;
        getJointWorldMatrices_(skinnedmeshes[i].root, lm::mat4(), all_matrices, joint_counter);
        
        //send to shader
        glUniformMatrix4fv(u_model, skinnedmeshes[i].num_joints, GL_FALSE, &all_matrices[0]);
        
        joint_shader_->setUniform(U_VP, cam.view_projection);
        
        glBindVertexArray(joints_vaos_[i]);
        glDrawElements(GL_LINES, skinnedmeshes[i].num_joints * 2 , GL_UNSIGNED_INT, 0);
    }
}

void DebugSystem::drawGrid_() {
    //get the camera view projection matrix
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    
    //use line shader to draw all lines and boxes
    glUseProgram(grid_shader_->program);
    GLint u_mvp = glGetUniformLocation(grid_shader_->program, "u_mvp");
    GLint u_color = glGetUniformLocation(grid_shader_->program, "u_color");
    GLint u_color_mod = glGetUniformLocation(grid_shader_->program, "u_color_mod");
    GLint u_size_scale = glGetUniformLocation(grid_shader_->program, "u_size_scale");
    GLint u_center_mod = glGetUniformLocation(grid_shader_->program, "u_center_mod");
    
    //set uniforms and draw grid
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, vp.m);
    glUniform3fv(u_color, 4, grid_colors);
    glUniform3f(u_size_scale, 1.0, 1.0, 1.0);
    glUniform3f(u_center_mod, 0.0, 0.0, 0.0);
    glUniform1i(u_color_mod, 0);
    glBindVertexArray(grid_vao_); //GRID
    glDrawElements(GL_LINES, grid_num_indices, GL_UNSIGNED_INT, 0);
}

void DebugSystem::drawFrusta_() {
    //get the camera view projection matrix
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    GLint u_mvp = glGetUniformLocation(grid_shader_->program, "u_mvp");
    GLint u_color_mod = glGetUniformLocation(grid_shader_->program, "u_color_mod");
    
    //draw frustra for all cameras
    auto& cameras = ECS.getAllComponents<Camera>();
    int counter = 0;
    for (auto& cc : cameras) {
        //don't draw current camera frustum
        if (counter == ECS.main_camera) continue;
        counter++;
        
        lm::mat4 cam_iv = cc.view_matrix;
        cam_iv.inverse();
        lm::mat4 cam_ip = cc.projection_matrix;
        cam_ip.inverse();
        lm::mat4 cam_ivp = cc.view_projection;
        cam_ivp.inverse();
        lm::mat4 mvp = vp * cam_ivp;
        
        //set uniforms and draw cube
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
        glUniform1i(u_color_mod, 1); //set color to index 1 (red)
        glBindVertexArray(cube_vao_); //CUBE
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }
}

void DebugSystem::drawColliders_() {
    //get the camera view projection matrix
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    GLint u_mvp = glGetUniformLocation(grid_shader_->program, "u_mvp");
    GLint u_color_mod = glGetUniformLocation(grid_shader_->program, "u_color_mod");
    
    //draw all colliders
    auto& colliders = ECS.getAllComponents<Collider>();
    for (auto& cc : colliders) {
        //get transform for collider
        Transform& tc = ECS.getComponentFromEntity<Transform>(cc.owner);
        //get the colliders local model matrix in order to draw correctly
        lm::mat4 collider_matrix = tc.getGlobalMatrix(ECS.getAllComponents<Transform>());
        
        if (cc.collider_type == ColliderTypeBox) {
            
            //now move by the box by its offset
            collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);
            //convert -1 -> +1 geometry to size of collider box
            collider_matrix.scaleLocal(cc.local_halfwidth.x, cc.local_halfwidth.y, cc.local_halfwidth.z);
            //set mvp
            lm::mat4 mvp = vp * collider_matrix;
            
            //set uniforms and draw
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
            glUniform1i(u_color_mod, 2); //set color to index 2 (green)
            glBindVertexArray(cube_vao_); //CUBE
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        }
        
        if (cc.collider_type == ColliderTypeRay) {
            //ray natively goes from (0,0,0 to 0,0,1) (see definition in createRay_())
            //we need to rotate this vector so that it matches the direction specified by the component
            //to do this we first need to find angle and axis between the two vectors
            lm::vec3 buffer_vec(0, 0, 1);
            lm::vec3 dir_norm = cc.direction;
            dir_norm.normalize();
            float rotation_angle = acos(dir_norm.dot(buffer_vec));
            //if angle is PI, vector is opposite to buffer vec
            //so rotation axis can be anything (we set it to 0,1,0 vector
            lm::vec3 rotation_axis = lm::vec3(0, 1, 0);
            //otherwise, we calculate rotation axis with cross product
            if (rotation_angle < 3.14159f) {
                rotation_axis = dir_norm.cross(buffer_vec).normalize();
            }
            //now we rotate the buffer vector to
            if (rotation_angle > 0.00001f) {
                //only rotate if we have to
                collider_matrix.rotateLocal(rotation_angle, rotation_axis);
            }
            //apply distance scale
            collider_matrix.scaleLocal(cc.max_distance, cc.max_distance, cc.max_distance);
            //apply center offset
            collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);
            
            //set uniforms
            lm::mat4 mvp = vp * collider_matrix;
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
            //set color to index 2 (green)
            glUniform1i(u_color_mod, 3);
            
            //bind the cube vao
            glBindVertexArray(collider_ray_vao_);
            glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);
        }
    }
}


void DebugSystem::drawIcons_() {
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    
    //switch to icon shader
    glUseProgram(icon_shader_->program);
    
    //get uniforms
    GLint u_mvp = glGetUniformLocation(icon_shader_->program, "u_mvp");
    GLint u_icon = glGetUniformLocation(icon_shader_->program, "u_icon");
    glUniform1i(u_icon, 0);
    
    
    //for each light - bind light texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, icon_light_texture_);
    
    auto& lights = ECS.getAllComponents<Light>();
    for (auto& curr_light : lights) {
        Transform& curr_light_transform = ECS.getComponentFromEntity<Transform>(curr_light.owner);
        
        lm::mat4 mvp_matrix = vp * curr_light_transform.getGlobalMatrix(ECS.getAllComponents<Transform>());;
        //BILLBOARDS
        //the mvp for the light contains rotation information. We want it to look at the camera always.
        //So we zero out first three columns of matrix, which contain the rotation information
        //this is an extremely simple billboard
        lm::mat4 bill_matrix;
        for (int i = 12; i < 16; i++) bill_matrix.m[i] = mvp_matrix.m[i];
        
        //send this new matrix as the MVP
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, bill_matrix.m);
        glBindVertexArray(icon_vao_);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    
    //bind camera texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, icon_camera_texture_);
    
    //for each camera, exactly the same but with camera texture
    auto& cameras = ECS.getAllComponents<Camera>();
    for (auto& curr_camera : cameras) {
        Transform& curr_cam_transform = ECS.getComponentFromEntity<Transform>(curr_camera.owner);
        lm::mat4 mvp_matrix = vp * curr_cam_transform.getGlobalMatrix(ECS.getAllComponents<Transform>());
        
        // billboard as above
        lm::mat4 bill_matrix;
        for (int i = 12; i < 16; i++) bill_matrix.m[i] = mvp_matrix.m[i];
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, bill_matrix.m);
        glBindVertexArray(icon_vao_);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
    }
}

// recursive function to render a transform node in imGUI
void DebugSystem::imGuiRenderTransformNode(TransformNode& trans) {
	

	auto& ent = ECS.entities[trans.entity_owner];
	if (ImGui::TreeNode(ent.name.c_str())) {
		Transform& transform = ECS.getComponentFromEntity<Transform>(ent.name);
		if (ECS.getComponentID<Light>(trans.entity_owner) != -1) {
			graphics_system_->needUpdateLights = true;
		}

		lm::vec3 pos = transform.position();
		float pos_array[3] = { pos.x, pos.y, pos.z };
		ImGui::DragFloat3("Position", pos_array);
		transform.position(pos_array[0], pos_array[1], pos_array[2]);

		
		if (ImGui::SliderFloat("X Rotation", &x_degree, 0.0f, 360.0f, " %0.3f", 1.0f)) {
			
			float grados = 0;
			if (x_degree >= f_Xdegree) {
				grados = x_degree - f_Xdegree;
				grados = grados * 0.01745;
				f_Xdegree = x_degree;
				transform.rotateLocal(grados, lm::vec3(1, 0, 0));
			}
			else {
				grados = f_Xdegree - x_degree;
				grados = grados * 0.01745;
				f_Xdegree = x_degree;
				transform.rotateLocal(grados, lm::vec3(-1, 0, 0));
			}
		}
		if (ImGui::SliderFloat("Y Rotation", &y_degree, 0.0f, 360.0f, "%0.3f", 1.0f)) {
			
			float grados = 0;
			if (y_degree >= f_Ydegree) {
				grados = y_degree - f_Ydegree;
				grados = grados * 0.01745;
				f_Ydegree = y_degree;
				transform.rotateLocal(grados, lm::vec3(0, 1, 0));
			}
			else {
				grados = f_Ydegree - y_degree;
				grados = grados * 0.01745;
				f_Ydegree = y_degree;
				transform.rotateLocal(grados, lm::vec3(0, -1, 0));
			}
		}
		if (ImGui::SliderFloat("Z Rotation", &z_degree, 0.0f, 360.0f, "%0.3f", 1.0f)) {
			
			float grados = 0;
			if (z_degree >= f_Zdegree) {
				grados = z_degree - f_Zdegree;
				grados = grados * 0.01745;
				transform.rotateLocal(grados, lm::vec3(0, 0, 1));
				f_Zdegree = z_degree;
			}
			else {
				grados = f_Zdegree - z_degree;
				grados = grados * 0.01745;
				transform.rotateLocal(grados, lm::vec3(0, 0, -1));
				f_Zdegree = z_degree;
			}
		}

		if (x_scale < 1 || y_scale < 1 || z_scale < 1 || f_Xscale < 1 || f_Yscale < 1 || f_Zscale < 1) {
			x_scale = y_scale = z_scale = f_Xscale = f_Yscale = f_Zscale = 1;
		}

		if (ImGui::SliderFloat("X Scale", &x_scale, 1.0f, 10.0f, " %0.3f", 1.0f)) {
			if (x_scale > f_Xscale) {
				if (x_scale >= 1) {
					transform.scaleLocal(1 / f_Xscale, 1, 1);
					transform.scaleLocal(x_scale, 1, 1);
					f_Xscale = x_scale;
				}
			}
			else {
				if (x_scale >= 1) {
					transform.scaleLocal(1 / f_Xscale, 1, 1);
					transform.scaleLocal(x_scale, 1, 1);
					f_Xscale = x_scale;
				}
			}
		}
		if (ImGui::SliderFloat("Y Scale", &y_scale, 1.0f, 10.0f, "%0.3f", 1.0f)) {
			if (y_scale > f_Yscale) {
				if (y_scale >= 1) {
					transform.scaleLocal(1, 1 / f_Yscale, 1);
					transform.scaleLocal(1, y_scale, 1);
					f_Yscale = y_scale;
				}
			}
			else {
				if (y_scale >= 1) {
					transform.scaleLocal(1, 1 / f_Yscale, 1);
					transform.scaleLocal(1, y_scale, 1);
					f_Yscale = y_scale;
				}
			}
		}
		if (ImGui::SliderFloat("Z Scale", &z_scale, 1.0f, 10.0f, "%0.3f", 1.0f)) {
			if (z_scale > f_Zscale) {
				if (z_scale >= 1) {
					transform.scaleLocal(1, 1, 1 / f_Zscale);
					transform.scaleLocal(1, 1, z_scale);
					f_Zscale = z_scale;
				}
			}
			else {
				if (z_scale >= 1) {
					transform.scaleLocal(1, 1, 1 / f_Zscale);
					transform.scaleLocal(1, 1, z_scale);
					f_Zscale = z_scale;
				}
			}
		}

		

		BlendShapes& blend_comp = ECS.getComponentFromEntity<BlendShapes>(ECS.getEntity("toon"));

		if (ECS.getEntity("toon") != -1) {
			if (blend_comp.owner == trans.entity_owner) {
				ImGui::SliderFloat("Happy", &blend_comp.blend_weights[0], 0, 1, 0);
				ImGui::SliderFloat("Angry", &blend_comp.blend_weights[1], 0, 1, 0);
			}
		}
	
		

		for (auto& child : trans.children) {

			imGuiRenderTransformNode(child);
		}
		ImGui::TreePop();
	}
}


//called at the end of DebugSystem::update()
void DebugSystem::updateimGUI_(float dt) {

    //show_imGUI_ is a bool toggled directly from Game, using Alt-0
	if (show_imGUI_)
	{
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//Demo window
//        ImGui::ShowDemoWindow();

		//get input
		ImGuiIO &io = ImGui::GetIO();

		//if imGUI wants the mouse, don't fire picking ray
        //this disables firing picking ray when pointer is
        //over imGUI window
		if (io.WantCaptureMouse)
			can_fire_picking_ray_ = false;
		else
			can_fire_picking_ray_ = true;

		//creamos la ventana para escribir los scritps
		static ExampleAppConsole console;
		console.orden = consoleScript;
		console.Draw("Console", &show_imGUI_);
		
		//acabamos la ventana para escribir los scripts

		//ventana de los materiales
		ImGui::SetNextWindowSize(ImVec2(400, 400));
		ImGui::SetNextWindowBgAlpha(1.0);
		ImGui::Begin("Materials", &show_imGUI_);

		auto& materiales = graphics_system_->getMaterials();

		for (auto& material : materiales) {
			std::string nombre = material.name;
			
			if (ImGui::TreeNode(nombre.c_str())) {
				
				graphics_system_->indice = material.index;
				
				//height------------------------------------------------
				ImGui::SliderFloat("Height", &material.height, -50, 50, "%0.3f");
				//graphics_system_->height = material.height;

				//uv scale----------------------------------------------
				//lm::vec2 uv = material.uv_scale;
				float uv[2] = { material.uv_scale.x, material.uv_scale.y };
				if (uv[0] < 1 || uv[1] < 1) {
					uv[0] = 1;
					uv[1] = 1;
				}
				if (ImGui::SliderFloat2("UV Scale", uv, 1, 500, 0)) {
					material.uv_scale.x = uv[0];
					material.uv_scale.y = uv[1];
					graphics_system_->uv = material.uv_scale;
				}

				//diffuse------------------------------------------------
				float diff[3] = { difuse.x, difuse.y, difuse.z };
				ImGui::DragFloat3("Diffuse", diff, 0.1f,  0.0f, 1.0f);
				difuse.x = diff[0];
				difuse.y = diff[1];
				difuse.z = diff[2];
				graphics_system_->difuse = difuse;
				
				//specular-----------------------------------------------
				float spec[3] = { specular.x, specular.y, specular.z };
				ImGui::DragFloat3("Specular", spec, 0.1f, 0.0f, 1.0f);
				specular.x = spec[0];
				specular.y = spec[1];
				specular.z = spec[2];
				graphics_system_->specular = specular;

				//ambient------------------------------------------------
				float amb[3] = { ambient.x, ambient.y, ambient.z };
				ImGui::DragFloat3("Ambient", amb, 0.1f, 0.0f, 1.0f);
				ambient.x = amb[0];
				ambient.y = amb[1];
				ambient.z = amb[2];
				graphics_system_->ambient = ambient;

				//specular gloss-----------------------------------------
				float gloss[1] = { spec_glos };
				ImGui::DragFloat("Specular Gloss", gloss, 0.1f, 0.1f, 10.0f);
				spec_glos = gloss[0];
				graphics_system_->spec_glos = spec_glos;


				//normal factor------------------------------------------
				float fact[1] = { normal_fact };
				ImGui::DragFloat("Normal Factor", fact, 0.1f, 0.0f, 1.0f);
				normal_fact = fact[0];
				graphics_system_->normal_fact = normal_fact;

				ImGui::TreePop();
			}
			
		}

		ImGui::End();
		//se acaba la ventana de los materiales

		//Ventana de las particulas
		ImGui::SetNextWindowSize(ImVec2(400, 400));
		ImGui::SetNextWindowBgAlpha(1.0);
		ImGui::Begin("Particulas", &show_imGUI_);
		auto& particulas = ECS.getAllComponents<ParticleEmitter>();

		for (auto& particula : particulas) {
			if (ImGui::TreeNode(particula.name.c_str())) {
				ImGui::SliderFloat("Particles Size", &particula.particle_size, 1, 10, "%0.3f");

				if (ImGui::SliderInt("Num Particles", &particula.num_particles, 1, 3000, "%0.3f")) {
					particula.entra = true;
				}
			}
		}
		ImGui::End();
		//se acaba la ventana de las aprticulas

		//ventana de la luz
		ImGui::SetNextWindowSize(ImVec2(400, 400));
		ImGui::SetNextWindowBgAlpha(1.0);
		ImGui::Begin("Lights", &show_imGUI_);

		auto& lights = ECS.getAllComponents<Light>();
		
		for (auto& light : lights) {
			std::string nombre = ECS.getEntityName(light.owner);

			if (ImGui::TreeNode(nombre.c_str())) {

				std::cout << ECS.getEntityName(light.owner) << std::endl;
				graphics_system_->luz = light.owner;

				if (ECS.getEntityName(light.owner) == nombre) {

					float dir[3] = { direction.x, direction.y, direction.z };
					ImGui::DragFloat3("Direction", dir, 0.1f, -1.0f, 1.0f);
					direction.x = dir[0]; direction.y = dir[1]; direction.z = dir[2];
					graphics_system_->direction = direction;

					float col[3] = { color.x, color.y, color.z };
					ImGui::DragFloat3("Color", col, 0.1f, 0.0f, 1.0f);
					color.x = col[0]; color.y = col[1]; color.z = col[2];
					graphics_system_->color = color;

					float lin[1] = { l_att };
					ImGui::DragFloat("Linear Att", lin, 0.1f, -1.0f, 1.0f);
					l_att = lin[0];
					graphics_system_->l_att = l_att;

					float quad[1] = { q_att };
					ImGui::DragFloat("Quadratic Att", quad, 0.1f, -1.0f, 1.0f);
					q_att = quad[0];
					graphics_system_->q_att = q_att;

					float in[1] = { p_inner };
					ImGui::DragFloat("Spot Inner", in, 0.1f, 0.0f, 20.0f);
					p_inner = in[0];
					graphics_system_->p_inner = p_inner;

					float out[1] = { s_outter };
					ImGui::DragFloat("Spot Outer", out, 0.1f, 0.0f, 20.0f);
					s_outter = out[0];
					graphics_system_->s_outter = s_outter;

					ImGui::Checkbox("Cast Shadows", &shadows);
					graphics_system_->shadows = shadows;

					graphics_system_->needUpdateLights = true;
				}

				ImGui::TreePop();
			}
		}

		ImGui::Checkbox("Deferred Rendering", &graphics_system_->deferred);

		ImGui::End();
		//se acaba la ventana de la luz

		//open window
		ImGui::SetNextWindowSize(ImVec2(400, 200));
		ImGui::SetNextWindowBgAlpha(1.0);
		ImGui::Begin("Scene", &show_imGUI_);
        
		//Tell imGUI to display variables of the camera
		//get camera and its transform
		Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
		Transform& cam_transform = ECS.getComponentFromEntity<Transform>(cam.owner);


		if (mS < 20) {
			mS = 20;
		}

		auto& anims = ECS.getAllComponents<Animation>();

		for (auto& anim : anims) {
			ImGui::SliderFloat(" Animation ms/frame", &mS, 20, 60, 0);
			anim.ms_frame = mS;
		}

		//Create an unfoldable tree node called 'Camera'
		if (ImGui::TreeNode("Camera")) {
			//create temporary arrays with position and direction data
			float cam_pos_array[3] = { cam.position.x, cam.position.y, cam.position.z };
			float cam_dir_array[3] = { cam.forward.x, cam.forward.y, cam.forward.z };

			//create imGUI components that allow us to change the values when click-dragging
			ImGui::DragFloat3("Position", cam_pos_array);
			ImGui::DragFloat3("Direction", cam_dir_array);

			//use values of temporary arrays to set real values (in case user changes)
			cam.position = lm::vec3(cam_pos_array[0], cam_pos_array[1], cam_pos_array[2]);
			cam_transform.position(cam.position);
			cam.forward = lm::vec3(cam_dir_array[0], cam_dir_array[1], cam_dir_array[2]).normalize();
			ImGui::TreePop();
		}

		//create a tree of TransformNodes objects (defined in DebugSystem.h)
        //which represents the current scene graph
        
        // 1) create a temporary array with ALL transforms
		std::vector<TransformNode> transform_nodes;
		auto& all_transforms = ECS.getAllComponents<Transform>();
		for (size_t i = 0; i < all_transforms.size(); i++) {
			TransformNode tn;
			tn.trans_id = (int)i;
			tn.entity_owner = all_transforms[i].owner;
			tn.ent_name = ECS.entities[tn.entity_owner].name;
			if (all_transforms[i].parent == -1)
				tn.isTop = true;
			transform_nodes.push_back(tn);
		}
        
		// 2) traverse array to assign children to their parents
		for (size_t i = 0; i < transform_nodes.size(); i++) {
			int parent = all_transforms[i].parent;
			if (parent != -1) {
				transform_nodes[parent].children.push_back(transform_nodes[i]);
			}
		}
        
		// 3) create a new array with only top level nodes of transform tree
		std::vector<TransformNode> transform_topnodes;
		for (size_t i = 0; i < transform_nodes.size(); i++) {
			if (transform_nodes[i].isTop)
				transform_topnodes.push_back(transform_nodes[i]);
		}

        //create 2 imGUI columns, first contains transform tree
        //second contains selected item from picking
		

		//draw all the nodes
		for (auto& trans : transform_topnodes) {
            //this is a recursive function (defined above)
            //which draws a transform node (and its children)
            //using imGUI
			imGuiRenderTransformNode(trans);
		}

        
        
        
        //*** PICKING*** //
        //general approach: Debug System has a member variable which is an entity
        //with Ray Collider (ent_picking_ray_). When user clicks on the screen, this
        //ray is fired into the scene.
        //if it collides with a box collider, we read that collision here and render
        //imGUI with the details of the collider
        
        //look at DebugSystem::setPickingRay_() to see how picking ray is constructed
        
        //next column for picking
		ImGui::NextColumn();
    
		//get the pick ray first
		Collider& pick_ray_collider = ECS.getComponentFromEntity<Collider>(ent_picking_ray_);

		//is it colliding? if so, get pitcked, entity, and transform, and render imGUI text
		int picked_entity = -1;
		if (pick_ray_collider.colliding) {
			//get the other collider and entity
			Collider& picked_collider = ECS.getComponentInArray<Collider>(pick_ray_collider.other);
			picked_entity = picked_collider.owner;
			Transform& picked_transform = ECS.getComponentFromEntity<Transform>(picked_entity);
			ImGui::Text("Selected entity:");
			ImGui::TextColored(ImVec4(1, 1, 0, 1), ECS.entities[picked_collider.owner].name.c_str());
		}


		ImGui::End();

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}

//this function takes a mouse screen point and fires a ray into the world
//using the inverse viewprojection matrux
void DebugSystem::setPickingRay(int mouse_x, int mouse_y, int screen_width, int screen_height) {
	
    //if we are not in debug mode (alt-0) do nothing!
	if (!can_fire_picking_ray_) return;

	//convert mouse_x and mouse_y to NDC
	float ndc_x = (((float)mouse_x / (float)screen_width) * 2) - 1;
	float ndc_y = (((float)(screen_height - mouse_y) / (float)screen_height) * 2) - 1;

    //start point for ray is point on near plane of ndc
	lm::vec4 mouse_near_plane(ndc_x, ndc_y, -1.0, 1.0);
	
    //get view projection
    Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
	lm::mat4 inv_vp = cam.view_projection;
	inv_vp.inverse();

    //get ray start point in world coordinates
    //don't forget this is in HOMOGENOUS coords :)
	lm::vec4 mouse_world = inv_vp * mouse_near_plane;
    //so we must normalize the vector
	mouse_world.normalize();
	lm::vec3 mouse_world_3(mouse_world.x, mouse_world.y, mouse_world.z);

    //set the picking ray
    //the actual collision detection will be done next frame in the CollisionSystem
	Transform& pick_ray_transform = ECS.getComponentFromEntity<Transform>(ent_picking_ray_);
	Collider& pick_ray_collider = ECS.getComponentFromEntity<Collider>(ent_picking_ray_);
	pick_ray_transform.position(cam.position);
	pick_ray_collider.direction = (mouse_world_3 - cam.position).normalize();
	pick_ray_collider.max_distance = 1000000;
}

///////////////////////////////////////////////
// **** Functions to create geometry ********//
///////////////////////////////////////////////

//creates a simple quad to store a light texture
void DebugSystem::createIcon_() {
	float is = 0.5f;
	GLfloat icon_vertices[12]{ -is, -is, 0, is, -is, 0, is, is, 0, -is, is, 0 };
	GLfloat icon_uvs[8]{ 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
	GLuint icon_indices[6]{ 0, 1, 2, 0, 2, 3 };
	glGenVertexArrays(1, &icon_vao_);
	glBindVertexArray(icon_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_vertices), icon_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//uvs
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_uvs), icon_uvs, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icon_indices), icon_indices, GL_STATIC_DRAW);
	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void DebugSystem::createRay_() {
	//4th component is color
	GLfloat icon_vertices[8]{ 0, 0, 0, 0,
		0, 0, 1, 0 };
	GLuint icon_indices[2]{ 0, 1 };
	glGenVertexArrays(1, &collider_ray_vao_);
	glBindVertexArray(collider_ray_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_vertices), icon_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icon_indices), icon_indices, GL_STATIC_DRAW);
	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void DebugSystem::createCube_() {

	//4th component is color!
	const GLfloat quad_vertex_buffer_data[] = {
		-1.0f,  -1.0f,  -1.0f,  0.0f,  //near bottom left
		1.0f,   -1.0f,  -1.0f,  0.0f,   //near bottom right
		1.0f,   1.0f,   -1.0f,  0.0f,    //near top right
		-1.0f,  1.0f,   -1.0f,  0.0f,   //near top left
		-1.0f,  -1.0f,  1.0f,   0.0f,   //far bottom left
		1.0f,   -1.0f,  1.0f,   0.0f,    //far bottom right
		1.0f,   1.0f,   1.0f,   0.0f,     //far top right
		-1.0f,  1.0f,   1.0f,   0.0f,    //far top left
	};

	const GLuint quad_index_buffer_data[] = {
		0,1, 1,2, 2,3, 3,0, //top
		4,5, 5,6, 6,7, 7,4, // bottom
		4,0, 7,3, //left
		5,1, 6,2, //right
	};

	glGenVertexArrays(1, &cube_vao_);
	glBindVertexArray(cube_vao_);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_buffer_data), quad_vertex_buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_index_buffer_data), quad_index_buffer_data, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//creates the debug grid for our scene
void DebugSystem::createGrid_() {

	std::vector<float> grid_vertices;
	const float size = 100.0f; //outer width and height
	const int div = 100; // how many divisions
	const int halfdiv = div / 2;
	const float step = size / div; // gap between divisions
	const float half = size / 2; // middle of grid

	float p; //temporary variable to store position
	for (int i = 0; i <= div; i++) {

		//lines along z-axis, need to vary x position
		p = -half + (i*step);
		//one end of line
		grid_vertices.push_back(p);
		grid_vertices.push_back(0);
		grid_vertices.push_back(half);
		if (i == halfdiv) grid_vertices.push_back(1); //color
		else grid_vertices.push_back(0);

		//other end of line
		grid_vertices.push_back(p);
		grid_vertices.push_back(0);
		grid_vertices.push_back(-half);
		if (i == halfdiv) grid_vertices.push_back(1); //color
		else grid_vertices.push_back(0);

		//lines along x-axis, need to vary z positions
		p = half - (i * step);
		//one end of line
		grid_vertices.push_back(-half);
		grid_vertices.push_back(0);
		grid_vertices.push_back(p);
		if (i == halfdiv) grid_vertices.push_back(3); //color
		else grid_vertices.push_back(0);

		//other end of line
		grid_vertices.push_back(half);
		grid_vertices.push_back(0);
		grid_vertices.push_back(p);
		if (i == halfdiv) grid_vertices.push_back(3); //color
		else grid_vertices.push_back(0);
	}

	//indices
	const int num_indices = (div + 1) * 4;
	GLuint grid_line_indices[num_indices];
	for (int i = 0; i < num_indices; i++)
		grid_line_indices[i] = i;

	grid_num_indices = num_indices;

	//gl buffers
	glGenVertexArrays(1, &grid_vao_);
	glBindVertexArray(grid_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, grid_vertices.size() * sizeof(float), &(grid_vertices[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(grid_line_indices), grid_line_indices, GL_STATIC_DRAW);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

