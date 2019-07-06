#include "Parsers.h"
#include <fstream>
#include <regex>
#include <unordered_map>
#include "extern.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "tinyxml2.h"

using namespace tinyxml2;

void split(std::string to_split, std::string delim, std::vector<std::string>& result) {
	size_t last_offset = 0;
	while (true) {
		//find first delim
		size_t offset = to_split.find_first_of(delim, last_offset); 
		result.push_back(to_split.substr(last_offset, offset - last_offset));
		if (offset == std::string::npos) // if at end of string
			break;
		else //otherwise continue
			last_offset = offset + 1;
	}
}

//replace all new lines and trim white space
std::string trim_white_space(std::string to_trim) {
    
    to_trim = std::regex_replace(to_trim, std::regex("\\n"), " ");
    while (to_trim.front() == ' ')
        to_trim = to_trim.erase(0,1);
    while (to_trim.back() == ' ')
        to_trim.pop_back();
    return to_trim;
}

void str_split_to_floats(std::string to_split, std::string delim, std::vector<float>& result) {
    size_t last_offset = 0;
    while (true) {
        //find first delim
        size_t offset = to_split.find_first_of(delim, last_offset);
        result.push_back((float)atof(to_split.substr(last_offset, offset - last_offset).c_str()));
        if (offset == std::string::npos) // if at end of string
            break;
        else //otherwise continue
            last_offset = offset + 1;
    }
}

void str_split_to_uints(std::string to_split, std::string delim, std::vector<GLuint>& result) {
    size_t last_offset = 0;
    while (true) {
        //find first delim
        size_t offset = to_split.find_first_of(delim, last_offset);
        result.push_back((GLuint)atoi(to_split.substr(last_offset, offset - last_offset).c_str()));
        if (offset == std::string::npos) // if at end of string
            break;
        else //otherwise continue
            last_offset = offset + 1;
    }
}

bool Parsers::parseMTL(std::string path, std::string filename, std::vector<Material>& materials, GLuint shader_id) {
    
    //first we sort the path out (because the filename might be a relative path
    //and we need the full path to read any MTL or texture files associated with the OBJ)
    size_t position_last_separator = filename.find_last_of("\\/");
    
    //if position_last_separator != null,
    if (std::string::npos != position_last_separator) {
        path += filename.substr(0, position_last_separator + 1); //add relative path to path, include slash
        filename = filename.substr(position_last_separator + 1); //remove relative path from filename
    }
    
    std::string line;
    std::ifstream file(path + filename);
    if (file.is_open()) {
        
        Material* curr_mat = nullptr;
        
        //parse file line by line
        while (std::getline(file, line)) {
            //split line string
            std::vector<std::string> words;
            split(line, " ", words);
            
            if (words.empty()) continue; //empty line, skip
            if (words[0][0] == '#') continue; //first word starts with #, line is a comment
            
            //create new material, set shader_id and render mode
            if (words[0] == "newmtl") {
                materials.emplace_back();
                curr_mat = &(materials.back());
                curr_mat->name = words[1];
                curr_mat->shader_id = shader_id;
            }
            if (words[0] == "Ka") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->ambient = lm::vec3((float)std::atof(words[1].c_str()),
                                             (float)std::atof(words[2].c_str()),
                                             (float)std::atof(words[3].c_str()));
            }
            if (words[0] == "Kd") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->diffuse = lm::vec3((float)std::atof(words[1].c_str()),
                                             (float)std::atof(words[2].c_str()),
                                             (float)std::atof(words[3].c_str()));
            }
            if (words[0] == "Ks") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->specular = lm::vec3((float)std::atof(words[1].c_str()),
                                              (float)std::atof(words[2].c_str()),
                                              (float)std::atof(words[3].c_str()));
            }
            if (words[0] == "Ns") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->specular_gloss = (float)std::atof(words[1].c_str());
            }
            if (words[0] == "map_Kd") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->diffuse_map = parseTexture(path + words[1]);
            }
            if (words[0] == "map_Bump") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->normal_map = parseTexture(path + words[1]);
            }
            if (words[0] == "map_Ks") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->specular_map = parseTexture(path + words[1]);
            }
            if (words[0] == "map_d") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->transparency_map = parseTexture(path + words[1]);
            }
        }
        file.close();
        return true;
    }
    std::cerr << "ERROR: Could read MTL file path, name: " << path << ", " << filename << std::endl;
    return false;
}

//parses a wavefront object into passed arrays
bool Parsers::parseOBJ(std::string filename, std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices) {
    
    vertices.clear();
    uvs.clear();
    normals.clear();
    indices.clear();
    
	std::string line;
	std::ifstream file(filename);
	if (file.is_open())
	{
		//declare containers for temporary and final attributes
		std::vector<lm::vec3> temp_vertices;
		std::vector<lm::vec2> temp_uvs;
		std::vector<lm::vec3> temp_normals;

		//container to store map for indices
		std::unordered_map<std::string, int> indices_map;
		int next_index = 0; //stores next available index

		//parse file line by line
		while (std::getline(file, line))
		{
			//split line string
			std::vector<std::string> words; 
			split(line, " ", words);
						
			if (words.empty()) continue; //empty line, skip
			if (words[0][0] == '#')	continue; //first word starts with #, line is a comment

			if (words[0] == "v") { //line contains vertex data
				//read words to floats
				int wn = 1;
				if (words[1] == "")
					wn = 2;

				lm::vec3 pos( (float)atof( words[wn].c_str() ),
							  (float)atof( words[wn+1].c_str() ),
							  (float)atof( words[wn+2].c_str() ) );
				//add to temporary vector of positions
				temp_vertices.push_back(pos);
			}
			if (words[0] == "vt") { //line contains texture data
				//read words to floats
				lm::vec2 tex( (float)atof(words[1].c_str() ),
							  (float)atof(words[2].c_str() ) );
				//add to temporary vector of texture coords
				temp_uvs.push_back(tex);
			}
			if (words[0] == "vn") { //line contains vertex data
				//read words to floats
				lm::vec3 norm( (float)atof(words[1].c_str() ),
							   (float)atof(words[2].c_str() ),
							   (float)atof(words[3].c_str() ) );
				//add to temporary vector of normals
				temp_normals.push_back(norm);
			}

			//line contains face-vertex data
			if (words[0] == "f") {
				if (words.size() < 4) continue; // faces with fewer than 3 vertices??!

				bool quad_face = false; //boolean to help us deal with quad faces

				std::vector<std::string> nums; // container used for split indices
				//for each face vertex
				for (int i = 1; i < words.size(); i++) {

					if (words[i] == "")
						continue;
					//check if face vertex has already been indexed
					if (indices_map.count(words[i]) == 0) {
					
						//if not, start by getting all indices
						nums.clear();
						split(words[i], "/", nums);
						int v_ind = atoi(nums[0].c_str()) - 1; //subtract 1 to convert to 0-based array!
						int t_ind = atoi(nums[1].c_str()) - 1;
						int n_ind = atoi(nums[2].c_str()) - 1;

						//add vertices to final arrays of floats
						for (int j = 0; j < 3; j++)
							vertices.push_back(temp_vertices[v_ind].value_[j]);
						for (int j = 0; j < 2; j++)
							uvs.push_back(temp_uvs[t_ind].value_[j]);
						for (int j = 0; j < 3; j++)
							normals.push_back(temp_normals[n_ind].value_[j]);
						
						//add an index to final array
						indices.push_back(next_index);

						//record that this index is used for this face vertex
						indices_map[words[i]] = next_index;

						//increment index
						next_index++;
					}
					else {
						//face vertex was already added to final arrays
						//so search for its stored index
						int the_index = indices_map.at(words[i]); //safe to use as we know it exists
						//add it to final index array
						indices.push_back(the_index);
					}

					//***CHECK FOR QUAD FACES****
					//If the face is a quads (i.e. words.size() == 5), we need to create two triangles
					//out of the quad. We have already created one triangle with words[1], [2] and [3]
					//now we make another with [4], [1] and [3]. 
					if (i == 4 ) { 
						//face-vertex 4 is already added, so we search for indices of 1 and 3 and add them
						int index_1 = indices_map.at(words[1]);
						indices.push_back(index_1);
						int index_3 = indices_map.at(words[3]);
						indices.push_back(index_3);
					}

				}

			}

		}
		file.close();
		return true;
	}
    std::string error_msg = "ERROR: Could not open file " + filename;
    print(error_msg);
	return false;
}


//parses a wavefront object into passed arrays
int Parsers::parseOBJ_multi(std::string filename, std::vector<Geometry>& geometries, std::vector<Material>& materials) {
    
	std::vector<GLfloat> vertices, uvs, normals;
	std::vector<GLuint> indices;

	std::string path;
	size_t position_last_separator = filename.find_last_of("\\/");
	if (std::string::npos != position_last_separator) {
		path += filename.substr(0, position_last_separator + 1);
		filename = filename.substr(position_last_separator + 1);
	}


	std::string line;
	std::ifstream file(path + filename);
	if (file.is_open())
	{
		//declare containers for temporary and final attributes
		std::vector<lm::vec3> temp_vertices;
		std::vector<lm::vec2> temp_uvs;
		std::vector<lm::vec3> temp_normals;

		//container to store map for indices
		std::unordered_map<std::string, int> indices_map;
		int next_index = 0; //stores next available index

		//current material id allows us to store material ids for sub-materials
		//it starts at -1, we set it as soon as we see a usemtl line
		int current_material_id = -1;
		bool usemtl = false; // used to say whether obj uses mtl or not

		//create 'empty' geometry
		geometries.emplace_back();
		Geometry* current_geometry = &(geometries.back());


		//parse file line by line
		while (std::getline(file, line))
		{
			//split line string
			std::vector<std::string> words;
			split(line, " ", words);

			if (words.empty()) continue; //empty line, skip
			if (words[0][0] == '#')    continue; //first word starts with #, line is a comment

			if (words[0] == "v") { //line contains vertex data
				//read words to floats
				lm::vec3 pos((float)atof(words[1].c_str()),
					(float)atof(words[2].c_str()),
					(float)atof(words[3].c_str()));
				//add to temporary vector of positions
				temp_vertices.push_back(pos);
			}
			if (words[0] == "vt") { //line contains texture data
				//read words to floats
				lm::vec2 tex((float)atof(words[1].c_str()),
					(float)atof(words[2].c_str()));
				//add to temporary vector of texture coords
				temp_uvs.push_back(tex);
			}
			if (words[0] == "vn") { //line contains vertex data
				//read words to floats
				lm::vec3 norm((float)atof(words[1].c_str()),
					(float)atof(words[2].c_str()),
					(float)atof(words[3].c_str()));
				//add to temporary vector of normals
				temp_normals.push_back(norm);
			}
			if (words[0] == "f") {//line contains face-vertex data
				if (words.size() < 4) continue; // faces with fewer than 3 vertices??!

				bool quad_face = false; //boolean to help us deal with quad faces

				std::vector<std::string> nums; // container used for split indices
				//for each face vertex
				for (int i = 1; i < words.size(); i++) {

					//check if face vertex has already been indexed
					if (indices_map.count(words[i]) == 0) {

						//if not, start by getting all indices
						nums.clear();
						split(words[i], "/", nums);
						int v_ind = atoi(nums[0].c_str()) - 1; //subtract 1 to convert to 0-based array!
						int t_ind = atoi(nums[1].c_str()) - 1;
						int n_ind = atoi(nums[2].c_str()) - 1;

						//add vertices to final arrays of floats
						for (int j = 0; j < 3; j++)
							vertices.push_back(temp_vertices[v_ind].value_[j]);
						for (int j = 0; j < 2; j++)
							uvs.push_back(temp_uvs[t_ind].value_[j]);
						for (int j = 0; j < 3; j++)
							normals.push_back(temp_normals[n_ind].value_[j]);

						//add an index to final array
						indices.push_back(next_index);

						//record that this index is used for this face vertex
						indices_map[words[i]] = next_index;

						//increment index
						next_index++;
					}
					else {
						//face vertex was already added to final arrays
						//so search for its stored index
						int the_index = indices_map.at(words[i]); //safe to use as we know it exists
						//add it to final index array
						indices.push_back(the_index);
					}

					//***CHECK FOR QUAD FACES****
					//If the face is a quads (i.e. words.size() == 5), we need to create two triangles
					//out of the quad. We have already created one triangle with words[1], [2] and [3]
					//now we make another with [4], [1] and [3].
					if (i == 4) {
						//face-vertex 4 is already added, so we search for indices of 1 and 3 and add them
						int index_1 = indices_map.at(words[1]);
						indices.push_back(index_1);
						int index_3 = indices_map.at(words[3]);
						indices.push_back(index_3);
					}

				}

			}
			if (words[0] == "usemtl") { //
				//we create material sets at the *end* of a list of faces
				//so if this is the first time we see this keyword, we do nothing

				if (!usemtl) // first time, do nothing
					usemtl = true;
				else //subsequent times, create material set for previous indices
					current_geometry->createMaterialSet((int)indices.size() / 3, current_material_id);

				//search materials array for mat with name
				//if not exists, current_material_id stays as -1
				for (int i = 0; i < materials.size(); i++) {
					if (materials[i].name == words[1]) {
						current_material_id = i;
						break;
					}

				}
			}


		}
		file.close();

		//create vertex arrays
		current_geometry->createVertexArrays(vertices, uvs, normals, indices);
		//close final (or only) material set and sets transparency flat
		current_geometry->createMaterialSet((int)indices.size() / 3, current_material_id);



		//return index of new geometry in the geometries array
		return (int)geometries.size() - 1;
	}
	return -1;
}

// load uncompressed RGB targa file into an OpenGL texture
GLint Parsers::parseTexture(std::string filename,
                            ImageData* image_data,
                            bool keep_data) {
    
	std::string str = filename;
	std::string ext = str.substr(str.size() - 4, 4);


	GLuint texture_id;

	if (ext == ".tga" || ext == ".TGA")
	{
		TGAInfo* tgainfo = loadTGA(filename);
		if (tgainfo == NULL) {
			std::cerr << "ERROR: Could not load TGA file" << std::endl;
			return false;
		}

		//generate new openGL texture and bind it (tell openGL we want to do stuff with it)
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id); //we are making a regular 2D texture

												  //screen pixels will almost certainly not be same as texture pixels, so we need to
												  //set some parameters regarding the filter we use to deal with these cases
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	//set the mag filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //set the min filter
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4); //use anisotropic filtering

																		  //this is function that actually loads texture data into OpenGL
		glTexImage2D(GL_TEXTURE_2D, //the target type, a 2D texture
			0, //the base level-of-detail in the mipmap
			(tgainfo->bpp == 24 ? GL_RGB : GL_RGBA), //specified the color channels for opengl
			tgainfo->width, //the width of the texture
			tgainfo->height, //the height of the texture
			0, //border - must always be 0
			(tgainfo->bpp == 24 ? GL_BGR : GL_BGRA), //the format of the incoming data
			GL_UNSIGNED_BYTE, //the type of the incoming data
			tgainfo->data); // a pointer to the incoming data

        //we want to use mipmaps
		glGenerateMipmap(GL_TEXTURE_2D);

        //clean up memory if required
        if (!keep_data) {
            delete tgainfo->data;
            delete tgainfo;
        }
        else {
            //keep image data for use in engine
            image_data->data = tgainfo->data;
            image_data->width = tgainfo->width;
            image_data->height = tgainfo->height;
            image_data->bytes_pp = tgainfo->bpp / 8;
            //delete tgainfo;
        }
		return texture_id;
	}
	else {
		std::cerr << "ERROR: No extension or extension not supported" << std::endl;
		return -1;
	}
}

// this reader supports only uncompressed RGB targa files with no colour table
TGAInfo* Parsers::loadTGA(std::string filename)
{
	//the TGA header is 18 bytes long. The first 12 bytes are for specifying the compression
	//and various fields that are very infrequently used, and hence are usually 0.
	//for this limited file parser, we start by reading the first 12 bytes and compare
	//them against the pattern that identifies the file a simple, uncompressed RGB file.
	//more info about the TGA format cane be found at http://www.paulbourke.net/dataformats/tga/

	char TGA_uncompressed[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	char TGA_compare[12];
	char info_header[6];
	GLuint bytes_per_pixel;
	GLuint image_size;

	//open file
	std::ifstream file(filename, std::ios::binary);

	//read first 12 bytes
	file.read(&TGA_compare[0], 12);
	std::streamsize read_header_12 = file.gcount();
	//compare to check that file in uncompressed (or not corrupted)
	int header_compare = memcmp(TGA_uncompressed, TGA_compare, sizeof(TGA_uncompressed));
	if (read_header_12 != sizeof(TGA_compare) || header_compare != 0) {
		std::cerr << "ERROR: TGA file is not in correct format or corrupted: " << filename << std::endl;
		file.close();
		return nullptr;
	}

	//read in next 6 bytes, which contain 'important' bit of header
	file.read(&info_header[0], 6);

	TGAInfo* tgainfo = new TGAInfo;

	tgainfo->width = info_header[1] * 256 + info_header[0]; //width is stored in first two bytes of info_header
	tgainfo->height = info_header[3] * 256 + info_header[2]; //height is stored in next two bytes of info_header

	if (tgainfo->width <= 0 || tgainfo->height <= 0 || (info_header[4] != 24 && info_header[4] != 32)) {
		file.close();
		delete tgainfo;
		std::cerr << "ERROR: TGA file is not 24 or 32 bits, or has no width or height: " << filename << std::endl;
		return NULL;
	}

	//calculate bytes per pixel and then total image size in bytes
	tgainfo->bpp = info_header[4];
	bytes_per_pixel = tgainfo->bpp / 8;
	image_size = tgainfo->width * tgainfo->height * bytes_per_pixel;

	//reserve memory for the image data
	tgainfo->data = (GLubyte*)malloc(image_size);

	//read data into memory
	file.read((char*)tgainfo->data, image_size);
	std::streamsize image_read_size = file.gcount();

	//check it has been read correctly
	if (image_read_size != image_size) {
		if (tgainfo->data != NULL)
			free(tgainfo->data);
		file.close();
		std::cerr << "ERROR: Could not read tga data: " << filename << std::endl;
		delete tgainfo;
		return NULL;
	}

	file.close();

	return tgainfo;
}

GLuint Parsers::parseCubemap(std::vector<std::string>& faces) {
    
    TGAInfo* tgainfo0 = loadTGA(faces[0]);
    TGAInfo* tgainfo1 = loadTGA(faces[1]);
    TGAInfo* tgainfo2 = loadTGA(faces[2]);
    TGAInfo* tgainfo3 = loadTGA(faces[3]);
    TGAInfo* tgainfo4 = loadTGA(faces[4]);
    TGAInfo* tgainfo5 = loadTGA(faces[5]);
    
    //set the member variables for easy access
    GLsizei width = (GLsizei)tgainfo0->width;
    GLsizei height = (GLsizei)tgainfo0->height;
    
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
    
    //Define all 6 faces
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo0->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo1->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo2->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo3->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo4->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo5->data);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 10);
    
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    
    
    //clean up memory
    delete tgainfo0->data; delete tgainfo0;
    delete tgainfo1->data; delete tgainfo1;
    delete tgainfo2->data; delete tgainfo2;
    delete tgainfo3->data; delete tgainfo3;
    delete tgainfo4->data; delete tgainfo4;
    delete tgainfo5->data; delete tgainfo5;
    
    
    
    return texture_id;
}

bool Parsers::parseJSONLevel(std::string filename,
                             GraphicsSystem& graphics_system, ControlSystem& control_system) {
    //read json file and stream it into a rapidjson document
    //see http://rapidjson.org/md_doc_stream.html
    std::ifstream json_file(filename);
    rapidjson::IStreamWrapper json_stream(json_file);
    rapidjson::Document json;
    json.ParseStream(json_stream);
    //check if its valid JSON
    if (json.HasParseError()) { std::cerr << "JSON format is not valid!" << std::endl;return false; }
    //check if its a valid scene file
    if (!json.HasMember("scene")) { std::cerr << "JSON file is incomplete! Needs entry: scene" << std::endl; return false; }
    if (!json.HasMember("directory")) { std::cerr << "JSON file is incomplete! Needs entry: directory" << std::endl; return false; }
    if (!json.HasMember("textures")) { std::cerr << "JSON file is incomplete! Needs entry: textures" << std::endl; return false; }
    if (!json.HasMember("materials")) { std::cerr << "JSON file is incomplete! Needs entry: materials" << std::endl; return false; }
    if (!json.HasMember("lights")) { std::cerr << "JSON file is incomplete! Needs entry: lights" << std::endl; return false; }
    if (!json.HasMember("entities")) { std::cerr << "JSON file is incomplete! Needs entry: entities" << std::endl; return false; }
    if (!json.HasMember("shaders")) { std::cerr << "JSON file is incomplete! Needs entry: shaders" << std::endl; return false; }
    
    
    printf("Parsing Scene Name = %s\n", json["scene"].GetString());
    
    std::string data_dir = json["directory"].GetString();
    
    //dictionaries
    std::unordered_map<std::string, int> geometries;
    std::unordered_map<std::string, GLuint> textures;
    std::unordered_map<std::string, int> materials;
    std::unordered_map<std::string, int> shaders;
    std::unordered_map<std::string, std::string> child_parent;
    
    //geometries
    for (rapidjson::SizeType i = 0; i < json["geometries"].Size(); i++) {
        //get values from json
        std::string name = json["geometries"][i]["name"].GetString();
        std::string file = json["geometries"][i]["file"].GetString();
        //load geometry
        int geom_id = graphics_system.createGeometryFromFile(data_dir + file);
        //add to dictionary
        geometries[name] = geom_id;
    }
    
    //shaders
    for (rapidjson::SizeType i = 0; i < json["shaders"].Size(); i++) {
        //get values from json
        std::string name = json["shaders"][i]["name"].GetString();
        std::string vertex = json["shaders"][i]["vertex"].GetString();
        std::string fragment = json["shaders"][i]["fragment"].GetString();
        //load shader
        Shader* new_shader = graphics_system.loadShader(vertex, fragment);
        new_shader->name = name;
        shaders[name] = new_shader->program;
        
    }
    
    //cameras
	if (json.HasMember("cameras")) {
		for (rapidjson::SizeType i = 0; i < json["cameras"].Size(); i++) {
			const std::string name = json["cameras"][i]["name"].GetString();
			const std::string type = json["cameras"][i]["name"].GetString();
			const std::string movement = json["cameras"][i]["movement"].GetString();
			auto& jp = json["cameras"][i]["position"];
			auto& jd = json["cameras"][i]["direction"];
			const float fov = json["cameras"][i]["fov"].GetFloat();
			const float near = json["cameras"][i]["near"].GetFloat();
			const float far = json["cameras"][i]["far"].GetFloat();

			int vp_w, vp_h; //get viewport dims from graphics system
			graphics_system.getMainViewport(vp_w, vp_h);


			if (movement == "free") {
				int ent_player = ECS.createEntity("PlayerFree");
				Camera& player_cam = ECS.createComponentForEntity<Camera>(ent_player);
				lm::vec3 the_position(jp[0].GetFloat(), jp[1].GetFloat(), jp[2].GetFloat());
				ECS.getComponentFromEntity<Transform>(ent_player).translate(the_position);
				player_cam.position = the_position;
				player_cam.forward = lm::vec3(jd[0].GetFloat(), jd[1].GetFloat(), jd[2].GetFloat());
				player_cam.setPerspective(fov*DEG2RAD, (float)vp_w / (float)vp_h, near, far);
				ECS.main_camera = ECS.getComponentID<Camera>(ent_player);
				control_system.control_type = ControlTypeFree;
			}
		}
	}
    
    //textures
    for (rapidjson::SizeType i = 0; i < json["textures"].Size(); i++) {
        //get values from json
        std::string name = json["textures"][i]["name"].GetString();
        
        //predeclare id
        GLuint tex_id = 0;
        
        //check if its an environment
        if (json["textures"][i].HasMember("files")) {
            std::vector<std::string> cube_faces{
                data_dir + json["textures"][i]["files"][0].GetString(),
                data_dir + json["textures"][i]["files"][1].GetString(),
                data_dir + json["textures"][i]["files"][2].GetString(),
                data_dir + json["textures"][i]["files"][3].GetString(),
                data_dir + json["textures"][i]["files"][4].GetString(),
                data_dir + json["textures"][i]["files"][5].GetString()
            };
            tex_id = parseCubemap(cube_faces);
        }
        else {
            //else it's a regular texture
            std::string file = json["textures"][i]["file"].GetString();
            //load texture
            tex_id = parseTexture(data_dir + file);
        }
        //add to dictionary
        textures[name] = tex_id;
    }
    
    //environment
    if (json.HasMember("environment")) {
        //get values from json
        std::string texture = json["environment"]["texture"].GetString();
        std::string geometry = json["environment"]["geometry"].GetString();
        std::string shader = json["environment"]["shader"].GetString();
        graphics_system.setEnvironment(textures[texture], geometries[geometry], shaders[shader]);
    }
    
    //materials
    for (rapidjson::SizeType i = 0; i < json["materials"].Size(); i++) {
        //get values from json
        std::string name = json["materials"][i]["name"].GetString();
        
        //create material
        int mat_id = graphics_system.createMaterial();
        
        //shader_id is mandatory
        graphics_system.getMaterial(mat_id).shader_id = shaders[json["materials"][i]["shader"].GetString()];
        
        //optional properties
        
        //diffuse texture
        if (json["materials"][i].HasMember("diffuse_map")) {
            std::string diffuse = json["materials"][i]["diffuse_map"].GetString();
            graphics_system.getMaterial(mat_id).diffuse_map = textures[diffuse]; //assign texture id from material
        }
        
        //diffuse
        if (json["materials"][i].HasMember("diffuse")) {
            auto& json_spec = json["materials"][i]["diffuse"];
            graphics_system.getMaterial(mat_id).diffuse = lm::vec3(json_spec[0].GetFloat(), json_spec[1].GetFloat(), json_spec[2].GetFloat());
        }
        else
            graphics_system.getMaterial(mat_id).diffuse = lm::vec3(1, 1, 1); //white diffuse
        
        
        //specular
        if (json["materials"][i].HasMember("specular")) {
            auto& json_spec = json["materials"][i]["specular"];
            graphics_system.getMaterial(mat_id).specular = lm::vec3(json_spec[0].GetFloat(), json_spec[1].GetFloat(), json_spec[2].GetFloat());
        }
        else
            graphics_system.getMaterial(mat_id).specular = lm::vec3(0, 0, 0); //no specular
        
        //ambient
        if (json["materials"][i].HasMember("ambient")) {
            auto& json_ambient = json["materials"][i]["ambient"];
            graphics_system.getMaterial(mat_id).ambient = lm::vec3(json_ambient[0].GetFloat(), json_ambient[1].GetFloat(), json_ambient[2].GetFloat());
        }
        else
            graphics_system.getMaterial(mat_id).ambient = lm::vec3(0.1f, 0.1f, 0.1f); //no specular
        
        //reflection
        if (json["materials"][i].HasMember("cube_map")) {
            std::string cube_map = json["materials"][i]["cube_map"].GetString();
            graphics_system.getMaterial(mat_id).cube_map = textures[cube_map];
        }
        
        //add to dictionary
        materials[name] = mat_id;
    }
    
	//lights
	for (rapidjson::SizeType i = 0; i < json["lights"].Size(); i++) {
		std::string light_name = json["lights"][i]["name"].GetString();
		std::string light_type = json["lights"][i]["type"].GetString();

		int ent_light = ECS.createEntity(light_name);
		ECS.createComponentForEntity<Light>(ent_light);

		auto& l = ECS.getComponentFromEntity<Light>(ent_light);

		//set type
		if (light_type == "directional") l.type = LightTypeDirectional;
		if (light_type == "point") l.type = LightTypePoint;
        if (light_type == "spot") l.type = LightTypeSpot;

		//color
		if (json["lights"][i].HasMember("color")) {
			auto json_lc = json["lights"][i]["color"].GetArray();
			l.color = lm::vec3(json_lc[0].GetFloat(), json_lc[1].GetFloat(), json_lc[2].GetFloat());
		}
		//transform
		if (json["lights"][i].HasMember("position")) {
			auto json_lp = json["lights"][i]["position"].GetArray();
			ECS.getComponentFromEntity<Transform>(ent_light).translate(json_lp[0].GetFloat(), json_lp[1].GetFloat(), json_lp[2].GetFloat());
		}
		//direction
		if (json["lights"][i].HasMember("direction")) {
			auto json_ld = json["lights"][i]["direction"].GetArray();
			l.direction = lm::vec3(json_ld[0].GetFloat(), json_ld[1].GetFloat(), json_ld[2].GetFloat());
		}
		//attenuation
		if (json["lights"][i].HasMember("linear_att"))
			l.linear_att = json["lights"][i]["linear_att"].GetFloat();
		if (json["lights"][i].HasMember("quadratic_att"))
			l.quadratic_att = json["lights"][i]["quadratic_att"].GetFloat();
		//spotlight params
		if (json["lights"][i].HasMember("spot_inner"))
			l.spot_inner = json["lights"][i]["spot_inner"].GetFloat();
		if (json["lights"][i].HasMember("spot_outer"))
			l.spot_outer = json["lights"][i]["spot_outer"].GetFloat();
	}
    
    //entities
    for (rapidjson::SizeType i = 0; i < json["entities"].Size(); i++) {
        
        //json for entity
        auto& json_ent = json["entities"][i];
        
        //get name
        std::string json_name = "";
        if (json_ent.HasMember("name"))
            json_name = json_ent["name"].GetString();
        
        //get geometry and material ids - obligatory fields
        std::string json_geometry = json_ent["geometry"].GetString();
        std::string json_material = json_ent["material"].GetString();
        
        //transform - obligatory field
        auto jt = json_ent["transform"]["translate"].GetArray();
        auto jr = json_ent["transform"]["rotate"].GetArray();
        auto js = json_ent["transform"]["scale"].GetArray();
        
        //create entity
        int ent_id = ECS.createEntity(json_name);
        Mesh& ent_mesh = ECS.createComponentForEntity<Mesh>(ent_id);
        ent_mesh.geometry = geometries[json_geometry];
        ent_mesh.material = materials[json_material];
        
        //transform
        auto& ent_transform = ECS.getComponentFromEntity<Transform>(ent_id);
        //rotate
        //get rotation euler angles
        lm::vec3 rotate; rotate.x = jr[0].GetFloat(); rotate.y = jr[1].GetFloat(); rotate.z = jr[2].GetFloat();
        //create quaternion from euler angles
        lm::quat qR(rotate.x*DEG2RAD, rotate.y*DEG2RAD, rotate.z*DEG2RAD);
        //create matrix which represents these rotations
        lm::mat4 R; R.makeRotationMatrix(qR);
        //multiply transform by this matrix
        ent_transform.set(ent_transform * R);
        
        //scale
        ent_transform.scaleLocal(js[0].GetFloat(), js[1].GetFloat(), js[2].GetFloat());
        //translate
        ent_transform.translate(jt[0].GetFloat(), jt[1].GetFloat(), jt[2].GetFloat());
        
        if (json_ent["transform"].HasMember("parent")) {
            std::string json_parent = json_ent["transform"]["parent"].GetString();
            if (json_name == "" || json_parent == "") std::cerr << "ERROR: Parser: Either parent or child has no name";
            child_parent[json_name] = json_parent;
        }
        
        
        //optional fields below
        if (json_ent.HasMember("collider")) {
            std::string coll_type = json_ent["collider"]["type"].GetString();
            if (coll_type == "Box") {
                Collider& box_collider = ECS.createComponentForEntity<Collider>(ent_id);
                box_collider.collider_type = ColliderTypeBox;
                
                auto json_col_center = json_ent["collider"]["center"].GetArray();
                box_collider.local_center.x = json_col_center[0].GetFloat();
                box_collider.local_center.y = json_col_center[1].GetFloat();
                box_collider.local_center.z = json_col_center[2].GetFloat();
                
                auto json_col_halfwidth = json_ent["collider"]["halfwidth"].GetArray();
                box_collider.local_halfwidth.x = json_col_halfwidth[0].GetFloat();
                box_collider.local_halfwidth.y = json_col_halfwidth[1].GetFloat();
                box_collider.local_halfwidth.z = json_col_halfwidth[2].GetFloat();
            }
            ///TODO - Ray
        }
    }
    
    //now link hierarchy need to get transform id from parent entity,
    //and link to transform object from child entity
    for (std::pair<std::string, std::string> relationship : child_parent)
    {
        //get parent entity
        int parent_entity_id = ECS.getEntity(relationship.second);
        Entity& parent = ECS.entities[parent_entity_id];
        int parent_transform_id = parent.components[0]; //transform component is always in slot 0
        
        //get child transform
        Transform& transform_child = ECS.getComponentFromEntity<Transform>(relationship.first);
        
        //link child transform with parent id
        transform_child.parent = parent_transform_id;
    }
    
    return true;
}

bool Parsers::parseAnimation(std::string filename) {
    
    std::string line;
    std::ifstream file(filename);
    int line_counter = 0;
    int frames_per_second = 0;
    if (file.is_open())
    {
        //get first line of file for target entity
        std::string target_ent = "";
        while (std::getline(file, line))
        {
            if (line_counter == 0){
                target_ent = line;
            }
            if (line_counter == 1){
                frames_per_second = stoi(line);
                break;
            }
            line_counter++;
        }
        
        if (target_ent != "") {
            int entity_id = ECS.getEntity(target_ent);
            if (entity_id == -1) {
                std::cout << "ERROR: entity does not exist in animation file" << filename << "\n";
                return false;
            }
            
            Animation& anim = ECS.createComponentForEntity<Animation>(entity_id);
            
            //parse rest of file line by line
            while (std::getline(file, line))
            {
                //split line string
                std::vector<std::string> w;
                split(line, " ", w);
                
                //empty model matrix for frame
                lm::mat4 new_frame;
                
                //make translation matrix
                lm::mat4 translation;
                translation.makeTranslationMatrix(atof(w[1].c_str()), atof(w[2].c_str()), atof(w[3].c_str()));
                
                //make rotation matrix
                lm::quat qrot(atof(w[4].c_str()), atof(w[5].c_str()), atof(w[6].c_str()));
                lm::mat4 rotation;
                rotation.makeRotationMatrix(qrot);
                
                //make scale matrix
                lm::mat4 scale;
                scale.makeScaleMatrix(atof(w[7].c_str()), atof(w[8].c_str()), atof(w[9].c_str()));
                
                //multiply the lot
                new_frame = translation * rotation * scale * new_frame;
                
                //add the keyframe
                anim.keyframes.push_back(new_frame);
            }
            //set number of keyframes
            anim.num_frames = (GLuint)anim.keyframes.size();
            anim.ms_frame = 1000.0 / (float)frames_per_second;
        }
        else {
            std::cout << "ERROR: animation file does not contain entity definition " << filename << "\n";
            return false;
        }
        
        
        return true;
    } else {
        std::cout << "ERROR: could not open file " << filename << "\n";
        return false;
    }
    
    
}

//returns newly created joint from node
Joint* parseXMLJointNode(XMLElement* curr_node,
                         std::unordered_map<std::string, Joint*>& id_pointer_map,
                         std::unordered_map<std::string, Joint*>& name_pointer_map,
                         std::unordered_map<std::string, lm::mat4>& bonename_bindpose,
                         Joint* parent_joint = nullptr) {
    //create new joint and set its parent
    Joint* new_joint = new Joint();
    new_joint->parent = parent_joint;
    new_joint->name = curr_node->Attribute("name");
    new_joint->id = curr_node->Attribute("id");
    
    //this is the index in the joint chain. Used for all sorts of stuff at rendering
    new_joint->index_in_chain = (GLint)id_pointer_map.size();
    
    //update map
    id_pointer_map[new_joint->id] = new_joint;
    name_pointer_map[new_joint->name] = new_joint;
    
    //set bind pose
    new_joint->bind_pose_matrix = bonename_bindpose[new_joint->name];
    
    //transform
    XMLElement* joint_matrix_node = curr_node->FirstChildElement("matrix");
    if (!joint_matrix_node) {
        print("ERROR: Joints transforms/matrices must be 'baked' by exporter");
        return nullptr;
    }
    std::vector<float> joint_matrix_floats;
    str_split_to_floats(trim_white_space(joint_matrix_node->GetText()), " ", joint_matrix_floats);
    lm::mat4 joint_mat(&joint_matrix_floats[0]);
    joint_mat.transpose(); // collada exports in row major, we are column major
    
    //copy matrix to new joint
    new_joint->matrix = joint_mat;
    new_joint->model_orig = joint_mat;
    
    //loop child nodes
    XMLElement* child = curr_node->FirstChildElement("node");
    for (child; child != NULL; child = child->NextSiblingElement("node")) {
        
        //if child is a joint
        std::string type = child->Attribute("type");
        if (type == "JOINT") {
            //parse the joint and add it to list of children;
            new_joint->children.push_back(parseXMLJointNode(child, id_pointer_map, name_pointer_map, bonename_bindpose, new_joint));
        }
    }
    return new_joint;
}

//opencollada parser
bool Parsers::parseCollada(std::string filename, Shader* shader, GraphicsSystem& graphics_system) {
    
    if (!shader) {
        print("Collada file must be associated with a shader!");
        return false;
    }
    
    //load document and check for errors
    XMLDocument doc;
    doc.LoadFile(filename.c_str());
    if (doc.Error()) {
        print("ERROR: Collada file not valid:");
        print(doc.ErrorStr());
        return false;
    }
    XMLElement* root = doc.FirstChildElement("COLLADA");
    if (!root) {
        print("ERROR: Collada file does not contain root COLLADA node");
        return false;
    }
    
    
    // map of geometry name to engine id
    std::unordered_map<std::string, GLuint> geometries;
    //the original vertex indices for each geometry (i.e. before creating openGL vertex
    std::unordered_map<std::string, std::vector<GLuint>> geometries_orig_vertex_indices;
    
    /***** GEOMETRIES ******/
    
    //definitions:
    //'original vertex' = vertex position ONLY in the original collada array
    //'opengl vertex' = a vertex sent to GPU featuring normal and texture coords
    
    //read all geometries, create opengl vertices with combinations of attributes,
    //mapping previously created vertices using string, much in the same way as the
    //.obj files.
    
    XMLElement* lib_geometries = root->FirstChildElement("library_geometries");
    if (lib_geometries){
        XMLElement* geom = lib_geometries->FirstChildElement("geometry");
        for (geom; geom != NULL; geom = geom->NextSiblingElement("geometry")) {
            //id and name
            std::string geom_id = geom->Attribute("id");
            std::string geom_name = geom->Attribute("name");
            
            //only one mesh element per geometry is supported
            XMLElement* mesh_element = geom->FirstChildElement("mesh");
            
            //read source data (for positions, normals, uvs) etc. to a string id
            std::unordered_map<std::string, std::vector<float>> sourceID_floatdata;
            
            XMLElement* mesh_source = mesh_element->FirstChildElement("source");
            for (mesh_source; mesh_source != NULL; mesh_source = mesh_source->NextSiblingElement("source")) {
                std::string mesh_source_id = mesh_source->Attribute("id");
                XMLElement* float_array_element = mesh_source->FirstChildElement("float_array");
                std::vector<float> raw_floats;
                std::string float_text = trim_white_space(float_array_element->GetText());
                str_split_to_floats(float_text, " ", raw_floats);
                sourceID_floatdata[mesh_source_id] = raw_floats;
            }
            
            //the position data is special for some unfathomable reason
            //and needs an extra mapping
            XMLElement* vertices_element = mesh_element->FirstChildElement("vertices");
            std::unordered_map<std::string, std::string> vertID_sourceID;
            std::string input_source = vertices_element->FirstChildElement("input")->Attribute("source");
            input_source = input_source.erase(0, 1);
            vertID_sourceID[vertices_element->Attribute("id")] = input_source;
            
            //check to see if we have only triangles, or mixed triangles/quads
            bool polylist = false;
            XMLElement* index_element = mesh_element->FirstChildElement("triangles");
            if (!index_element) {
                index_element = mesh_element->FirstChildElement("polylist");
                polylist = true;
            }
            //quick error check
            if (!index_element) {
                print("ERROR: No triangle or polylist was found in Collada file");
                return 0;
            }
            
            //we know parse the <input> semantic data to identify which stream of floats
            //belongs to which attribute. The arrays below will be filled
            std::vector<float> positions_raw_array;
            std::vector<float> normals_raw_array;
            std::vector<float> uvs_raw_array;
            
            XMLElement* input_element = index_element->FirstChildElement("input");
            for (input_element; input_element != NULL; input_element = input_element->NextSiblingElement("input")) {
                std::string semantic = input_element->Attribute("semantic");
                if (semantic == "VERTEX") {
                    std::string vertex_src_intermediate_id = input_element->Attribute("source");
                    vertex_src_intermediate_id = vertex_src_intermediate_id.erase(0, 1);
                    std::string vertex_src_id = vertID_sourceID[vertex_src_intermediate_id];
                    positions_raw_array = sourceID_floatdata[vertex_src_id];
                }
                else if (semantic == "NORMAL") {
                    std::string normal_src_id = input_element->Attribute("source");
                    normal_src_id = normal_src_id.erase(0, 1);
                    normals_raw_array = sourceID_floatdata[normal_src_id];
                }
                else if (semantic == "TEXCOORD") {
                    std::string uv_src_id = input_element->Attribute("source");
                    uv_src_id = uv_src_id.erase(0, 1);
                    uvs_raw_array = sourceID_floatdata[uv_src_id];
                }
            }
            
            //arrays are now filled with raw data. Time to create opengl vertices
            //we do this by parsing the count or vcount element
            
            
            GLuint num_indices = 0;
            int num_faces = std::stoi(index_element->Attribute("count"));
            std::vector<GLuint> vcounts;
            if (polylist) {
                //could be a mix of tris and quads e.g. 3 3 3 3 4 4 4 4 3 4 3 4 3
                //so need to parse entire array and add the total indices
                XMLElement* vcount = index_element->FirstChildElement("vcount");
                std::string vcount_string = trim_white_space(vcount->GetText());
                str_split_to_uints(vcount_string, " ", vcounts);
                //sum the lot
                for (auto i : vcounts)
                    num_indices += i;
                //quick error check
                if (num_indices == 0) {print("ERROR: No indices found in Collada polylist!");return 0;}
            }
            else { //triangles!
                num_indices = num_faces * 3;
            }
            
            //create 'intermediate format' of indices, similar to OBJ format
            //fill indices_raw_face with a string which contains indices of each vertex
            //where a '/' separates position/normal/uv and where ' ' separates vertices
            //so will either be 3 or entries
            //"0/0/20 20/20/3 0/20/34" or
            //"0/0/20 20/20/3 0/20/34 0/0/20"
            
            XMLElement* p_element = index_element->FirstChildElement("p");
            std::vector<std::string> indices_raw;
            std::string indices_text = trim_white_space(p_element->GetText());
            split(indices_text, " ", indices_raw);
            std::vector<std::string> indices_raw_face; // set of indices per face (either 3 or 4)
            indices_raw_face.resize(num_faces);
            
            int current_loc = 0;
            for (int i = 0; i < num_faces; i++) {
                std::string to_add = "";
                int num_indices_this_face = 3; //triangles
                if (polylist){ //mix tris/quads
                    num_indices_this_face = vcounts[i];
                }
                for (int j = 0; j < num_indices_this_face; j++) {
                    to_add += indices_raw[current_loc++] + "/";
                    to_add += indices_raw[current_loc++] + "/";
                    to_add += indices_raw[current_loc++];
                    if (j != num_indices_this_face - 1) to_add += " "; // separate vertoces
                }
                indices_raw_face[i] = to_add;
            }
            
            //right, now we have parsed the collada to an intermediate format
            //where one string = one face
            //lets parse that format to create final opengl vertices
            
            std::vector<float> positions_final;
            std::vector<float> normals_final;
            std::vector<float> uvs_final;
            std::vector<GLuint> indices_final;
            
            //map from final index to original position index
            //we will add this completed array to geometries_orig_vertex_indices
            //and refer to it when adding the animation weights for each vertex
            std::vector<GLuint> orig_indices;
            
            //store previously seen indices - the standard thing for creating an index buffer
            std::unordered_map<std::string, GLuint> index_map;
            
            GLuint next_index = 0;
            
            //advanced index by index, each index comprises 3 ints, which refer
            //to index of position, normal and uvs
            for (size_t i = 0; i < num_faces; i++) {
                
                //break face string into its constituent vertices
                std::vector<std::string> face_indices;
                split(indices_raw_face[i], " ", face_indices);
                
                //for each vertex of face
                for (size_t j = 0; j < face_indices.size(); j++) {
                    
                    //get indices to position/normal/texture array
                    std::vector<GLuint> current_index_ints;
                    str_split_to_uints(face_indices[j], "/", current_index_ints);
                    
                    float x, y, z, nx, ny, nz, u, v;
                    int ind_pos, ind_norm, ind_uv;
                    
                    ind_pos = current_index_ints[0];
                    ind_norm = current_index_ints[1];
                    ind_uv = current_index_ints[2];
                    
                    //if we have not seen this unique vertex before
                    //add it to final array
                    if (index_map.count(face_indices[j]) == 0) {
                        x = positions_raw_array[ind_pos * 3];
                        y = positions_raw_array[ind_pos * 3 + 1];
                        z = positions_raw_array[ind_pos * 3 + 2];
                        
                        nx = normals_raw_array[ind_norm * 3];
                        ny = normals_raw_array[ind_norm * 3 + 1];
                        nz = normals_raw_array[ind_norm * 3 + 2];
                        
                        u = uvs_raw_array[ind_uv * 2];
                        v = uvs_raw_array[ind_uv * 2 + 1];
                        
                        positions_final.push_back(x);
                        positions_final.push_back(y);
                        positions_final.push_back(z);
                        
                        normals_final.push_back(nx);
                        normals_final.push_back(ny);
                        normals_final.push_back(nz);
                        
                        uvs_final.push_back(u);
                        uvs_final.push_back(v);
                        
                        indices_final.push_back(next_index);
                        
                        //store index for index buffer
                        index_map[face_indices[j]] = next_index;
                        
                        //store initial position for the new index
                        orig_indices.push_back(ind_pos);
                        
                        next_index++;
                    } else {
                        //otherwise add index of previous time we saw it
                        indices_final.push_back(index_map[face_indices[j]]);
                    }
                    
                    //now deal with case where j = 4 => quad face
                    //we already made a face with indices 0, 1, and 2
                    //now going to make one with 3, 0, 2
                    if (j == 3) {
                        //face-vertex 3 is already added, so we search for indices of 0 and 2 and add them
                        int index_0 = index_map.at(face_indices[0]);
                        indices_final.push_back(index_0);
                        int index_2 = index_map.at(face_indices[2]);
                        indices_final.push_back(index_2);
                    }
                    
                }
                
            }
            
            //create the new geometry in the graphics system
            int new_geom_id = graphics_system.createGeometry(positions_final, uvs_final, normals_final, indices_final);
            
            //store the geometry id string with the engine id int
            //and save the map back to the original positions indices (for animation weights)
            geometries[geom_id] = new_geom_id;
            geometries_orig_vertex_indices[geom_id] = orig_indices;
        }
    }
    else {
        print("Collada file has no geometries!");
    }
    
    /***** MATERIALS - EFFECTS ******/
    
    //the materials system has two elements. Nodes can reference a 'material' which references an 'effect'
    //so need to read both and map them
    
    //maps effect ID string to engine material index - used later to connect material to mesh
    std::unordered_map<std::string, int> materials;
    //maps material ID string to effect ID string. Only used in to link collada materials->effects
    std::unordered_map<std::string, std::string> materialID_effectID;
    
    //materials don't really do anything apart from reference an effect, but need to read them,
    // as nodes reference them directly
    XMLElement* library_materials = root->FirstChildElement("library_materials");
    if (library_materials) {
        
        XMLElement* material_element = library_materials->FirstChildElement("material");
        for (material_element; material_element != NULL; material_element = material_element->NextSiblingElement("material")) {
            XMLElement* instance_effect = material_element->FirstChildElement("instance_effect");
            std::string instance_effect_url = instance_effect->Attribute("url");
            instance_effect_url = instance_effect_url.erase(0, 1);
            
            std::string material_element_id = material_element->Attribute("id");
            
            //map material to effect
            materialID_effectID[material_element_id] = instance_effect_url;
            
        }
    }
    
    //read effects - this is where the meat is
    XMLElement* lib_effects = root->FirstChildElement("library_effects");
    if (lib_effects) {
        
        XMLElement* effect = lib_effects->FirstChildElement("effect");
        for (effect; effect != NULL; effect = effect->NextSiblingElement("effect")) {
            
            //create new engine material here, for each effect
            int new_mat_id = graphics_system.createMaterial();
            Material& new_mat = graphics_system.getMaterial(new_mat_id);
            new_mat.shader_id = shader->program;
            
            std::string effect_id = effect->Attribute("id");
            
            //read effect properties and add them to engine material created above
            XMLElement* profile_common_element = effect->FirstChildElement("profile_COMMON");
            if (profile_common_element) {
                XMLElement* technique_element = profile_common_element->FirstChildElement("technique");
                if (technique_element) {
                    //check if we have phong or lambert or whatever.
                    //this needs to be more robust for future use!
                    XMLElement* effect_name_element = technique_element->FirstChildElement("lambert");
                    if (!effect_name_element) effect_name_element = technique_element->FirstChildElement("phong");
                    
                    if (effect_name_element) {
                        XMLElement* diffuse_element = effect_name_element->FirstChildElement("diffuse");
                        if (diffuse_element) {
                            XMLElement* color_element = diffuse_element->FirstChildElement("color");
                            std::vector<float> df;
                            std::string diffuse_string = trim_white_space(color_element->GetText());
                            str_split_to_floats(diffuse_string, " ", df);
                            new_mat.diffuse.x = df[0];
                            new_mat.diffuse.y = df[1];
                            new_mat.diffuse.z = df[2];
                        }
                    }
                }
            }
            //map effect id string to engine material index
            materials[effect_id] = new_mat_id;
        }
    }
    else {
        print("Collada file has no materials! Using whatever material we find...");
    }
    
    
    /*** LIBRARY CONTROLLERS - SKINS ***/
    
    //How does a controller work? A controller is essentially a skin.
    //There is usually ONE controller for every geometry.
    //Controller:
    // - Skin
    // - - bind shape matrix: "model matrix" for skin
    // - - source
    // - - - Name_array/float_array: raw data
    // - - - TechniqueCommon->accessor->param: data type
    
    //we must read TechniqueCommon->accessor->param in order to know what the
    //raw data of Name_array/float array is
    
    //Name_array: list of all bones *that affect this skin*
    //IMPORTANT NOTE: this list may be of different size to list of all bones!
    //we must store this list and then MAP the name of the bone to the index
    //of the bone in the final bone chain.
    
    //float_array (float) : vertex weights
    //float_array (float4x4): bind pose matrix *for each bone*
    
    
    //maps the id of controller source to a base transform
    std::unordered_map<std::string, lm::mat4> controllersourceID_transform;
    //maps each bonename to a bindpose matrix
    std::unordered_map<std::string, lm::mat4> bonename_bindpose;
    //maps controller id to geometry id
    std::unordered_map<std::string, std::string> controllerID_geomID;
    
    //variables to store vertex weights and joint IDs. these will be used to
    //send them to the GPU later
    //we store them here because the joint ids will have to be converted to
    //their index in the chain for everything to work correctly
    std::unordered_map<GLuint, std::vector<lm::vec4>> geomID_vertexWeights;
    std::unordered_map<GLuint, std::vector<std::vector<std::string>>> geomID_vertexJointIDs; //to map bones to final bone chain later!
    
    XMLElement* lib_controllers = root->FirstChildElement("library_controllers");
    if (lib_controllers) {
        XMLElement* controller = lib_controllers->FirstChildElement("controller");
        for (controller; controller != NULL; controller = controller->NextSiblingElement("controller")) {
            std::string controller_id = controller->Attribute("id");
            XMLElement* skin = controller->FirstChildElement("skin");
            std::string skin_source_id = skin->Attribute("source");
            skin_source_id = skin_source_id.erase(0, 1);
            if (skin) {
                //read all raw source data (bone names, bind pose, vertex weights)
                
                int bone_count = -1;
                std::vector<std::string> bone_names;
                std::vector<float> bind_pose_raw;
                std::vector<float> vertex_weights_raw;
                
                XMLElement* skin_source = skin->FirstChildElement("source");
                for (skin_source; skin_source != NULL; skin_source = skin_source->NextSiblingElement("source")) {
                    XMLElement* technique_common = skin_source->FirstChildElement("technique_common");
                    XMLElement* accessor = technique_common->FirstChildElement("accessor");
                    XMLElement* param = accessor->FirstChildElement("param");
                    
                    std::string source_type = param->Attribute("type");
                    if (source_type == "name") {
                        //get bone names and number of them
                        XMLElement* name_array = skin_source->FirstChildElement("Name_array");
                        std::string name_array_text = trim_white_space(name_array->GetText());
                        bone_count = std::stoi(name_array->Attribute("count"));
                        split(name_array_text, " ", bone_names);
                    }
                    
                    if (source_type == "float4x4") {
                        XMLElement* float_array = skin_source->FirstChildElement("float_array");
                        std::string float_array_text = trim_white_space(float_array->GetText());
                        //print(float_array_text);
                        str_split_to_floats(float_array_text, " ", bind_pose_raw);
                    }
                    
                    if (source_type == "float") {
                        XMLElement* float_array = skin_source->FirstChildElement("float_array");
                        std::string float_array_text = trim_white_space(float_array->GetText());
                        str_split_to_floats(float_array_text, " ", vertex_weights_raw);
                    }
                }
                
                //after reading source data, parse it into structures
                
                //VERTEX WEIGHTS
                XMLElement* vertex_weights_node = skin->FirstChildElement("vertex_weights");
                int num_vertices = std::stoi(vertex_weights_node->Attribute("count"));
                //number of joints affecting each vert
                XMLElement* vcount_node = vertex_weights_node->FirstChildElement("vcount");
                std::vector<unsigned int> num_joints_affecting_vertex;
                
                std::string vcount_node_text = trim_white_space(vcount_node->GetText());
                str_split_to_uints(vcount_node_text, " ", num_joints_affecting_vertex);
                
                //BIND POSES
                
                //overall bind matrix
                XMLElement* matrix_element = skin->FirstChildElement("bind_shape_matrix");
                if (matrix_element) {
                    std::vector<float> transform_floats;
                    std::string float_text = trim_white_space(matrix_element->GetText());
                    str_split_to_floats(float_text, " ", transform_floats);
                    lm::mat4 transform_temp(&transform_floats[0]);
                    transform_temp.transpose();
                    controllersourceID_transform[controller_id] = transform_temp;
                } else {
                    std::string err = "ERROR: Collada parser couldn't find bind matrix for controller " + controller_id;
                    print(err); return false;
                }
                //individual bind poses
                if (bone_count != -1) {
                    for (int i = 0; i < bone_count; i++) {
                        lm::mat4 this_bone_transform(&bind_pose_raw[i*16]);
                        this_bone_transform.transpose();
                        bonename_bindpose[bone_names[i]] = this_bone_transform;
                    }
                }
                else {
                    std::string err = "ERROR: Collada parser didn't parse controller bones correctly";
                    print(err);
                    return false;
                }
                
                
                /*** WEIGHT ARRAYS ****/
                //Works like this:
                //Collada references weights *per original vertex id*, Given that our mesh uses a pos/norm/uv
                //index buffer, it's extremely likely that the number of unique pos/norm/uv vertices is
                //greater that the number of original vertex ids.
                //So when we read the geometry above, we stored, in an std::vector, the original
                //index of the collada vertex, for each pos/norm/uv vertex (hereby called 'opengl' vertex)
                //
                //Well, collada references the weights per joint per vertex like this
                //[joint_id weight_id ... N] <- one vertex, where N is number of joints per vertex
                //there are M of these sets, where M is number of original (collada) vertices
                //
                //Our approach:
                //read all weight for all joints of original vertices
                //then right at the end use the orig_indices map to convert to arrays for the opengl indices
                
                
                //arrays to store weights and joint ids. One vec4 and string array per *original* vertex
                std::vector<lm::vec4> orig_vertex_weights_vec4(num_vertices, lm::vec4(0,0,0,0));
                //an 2D array of strings. Second dimension is always size=4 (max 4 weights per vertex)
                std::vector<std::vector<std::string>>
                orig_vertex_jointsids_string(num_vertices, std::vector<std::string>(4, ""));
                
                
                //v is an array of pairs, where each pair is joint_id:vertex_weight_id
                //v is split into groups of 'x' where 'x' is the value in vertex_weights
                //for each vertex
                std::vector<unsigned int> v_raw;
                XMLElement* v_node = vertex_weights_node->FirstChildElement("v");
                str_split_to_uints(trim_white_space(v_node->GetText()), " ", v_raw);
                
                //iterate all original vertices
                int v_counter = 0; //track where we are in the v_array
                for (int i = 0; i < num_vertices; i++){
                    //get number of joints affecting vertex
                    int joints_this_vert = num_joints_affecting_vertex[i];
                    //iterate all joints affecting vertex
                    for (int j = 0; j < joints_this_vert; j++) {
                        int joint_id = v_raw[v_counter++];
                        int vertex_weight_id = v_raw[v_counter++];
                        
                        //i is vertex id, j will be between 0 - 4
                        orig_vertex_weights_vec4[i].value_[j] = vertex_weights_raw[vertex_weight_id];
                        orig_vertex_jointsids_string[i][j] = bone_names[joint_id];
                        
                    }
                }
                
                //now transfer stuff from original collada position indices to final opengl vertex indices
                
                //map from original
                auto& orig_indices = geometries_orig_vertex_indices[skin_source_id];
                GLuint num_opengl_vertices = (GLuint)orig_indices.size();
                
                //final arrays that will be passed to GPU
                std::vector<lm::vec4> opengl_vertex_weights_vec4(num_opengl_vertices, lm::vec4(0,0,0,0));
                std::vector<std::vector<std::string>>
                opengl_vertex_jointsids_string(num_opengl_vertices, std::vector<std::string>(4, ""));
                
                //copy from original indices to opengl indices
                for (int i = 0; i < num_opengl_vertices; i++){
                    GLuint i_orig = orig_indices[i];
                    opengl_vertex_weights_vec4[i] = orig_vertex_weights_vec4[i_orig];
                    opengl_vertex_jointsids_string[i] = orig_vertex_jointsids_string[i_orig];
                }
                
                //store these weights and ids arrays in a map, associated with engine geometry index
                geomID_vertexWeights[geometries[skin_source_id]] = opengl_vertex_weights_vec4;
                geomID_vertexJointIDs[geometries[skin_source_id]] = opengl_vertex_jointsids_string;
                
                //FINALLY: map this skin to the geometry id, to use later
                controllerID_geomID[controller_id] = skin_source_id;
                
            }
            
        }
        
        //end controller
    }
    
    
    
    /*** VISUAL SCENES ***/
    
    //The visual scene is the 'content' (think entities) of scene
    //visual scene will contain
    // - matrix
    // - node (joints)
    // - node (skin)
    // OR
    // - node (geometry)
    //need to read nodes into containers and then link them afterwards (ufffff!)
    
    /* nodes in scene */
    XMLElement* lib_vis_scenes = root->FirstChildElement("library_visual_scenes");
    if (!lib_vis_scenes) {
        print("Collada file does not contain a library of visual scenes");
        return true;
    }
    //we only support one visual scene at the moment
    XMLElement* vis_scene = lib_vis_scenes->FirstChildElement("visual_scene");
    if (!vis_scene) {
        print("Collada file does not contain a visual scene");
        return true;
    }
    
    std::unordered_map<std::string, Joint*> jointID_jointPointer; //map joint id to pointer
    std::unordered_map<std::string, Joint*> jointName_jointPointer; //map joint name to pointer
    std::unordered_map<int, std::string> entID_jointID; //map entity id to joint ID (use above to get pointer)
    
    XMLElement* child = vis_scene->FirstChildElement("node");
    for (child; child != NULL; child = child->NextSiblingElement("node")) {
        
        //joint chain
        std::string name = child->Attribute("name");
        std::string jointID = child->Attribute("id");
        const XMLAttribute* type_attribute = child->FindAttribute("type");
        std::string type_attribute_string = "";
        if (type_attribute)
            type_attribute_string = type_attribute->Value();
        
        if ( type_attribute_string  == "JOINT") {
            //recursive function creates a new joint object
            //maps its pointer to its id and also to its name
            //also assigns it's bind pose
            //IMPORTANT: this function does a depth-first traversal of the joint tree, in the order specified by
            //the collada file. THE INDEX OF EACH JOINT is the order in which it is traversed here.
            Joint* root_joint = parseXMLJointNode(child, jointID_jointPointer, jointName_jointPointer, bonename_bindpose);
        }
        // regular node with either geometry or skin
        else {
            int new_entity = ECS.createEntity(name);
            
            //set model matrix
            XMLElement* matrix_element = child->FirstChildElement("matrix");
            if (matrix_element) {
                std::string matrix_sid = matrix_element->Attribute("sid");
                if (matrix_element && matrix_sid == "transform") {
                    std::vector<float> transform_floats;
                    std::string float_text = trim_white_space(matrix_element->GetText());
                    str_split_to_floats(float_text, " ", transform_floats);
                    lm::mat4 transform_temp(&transform_floats[0]);
                    transform_temp.transpose();
                    ECS.getComponentFromEntity<Transform>(new_entity).set(transform_temp);
                } else {
                    std::string err = "ERROR: Collada parser couldn't find transform matrix for node " + name;
                    print(err);
                    return false;
                }
            }
            
            //empty to start, as could be assigned either to controller (skin) or basic geometry
            //materials will be added to Mesh/Skinned Mesh after geometry is created
            XMLElement* bind_material_element = nullptr;
            
            //skinned mesh
            XMLElement* instance_controller = child->FirstChildElement("instance_controller");
            if (instance_controller) {
                std::string url = instance_controller->Attribute("url"); //this is id of the geometry in library_geometries
                url = url.erase(0, 1);
                std::string geomID = controllerID_geomID[url];
                
                SkinnedMesh& new_mesh = ECS.createComponentForEntity<SkinnedMesh>(new_entity);
                new_mesh.geometry = geometries[geomID];
                new_mesh.skin_bind_matrix = controllersourceID_transform[url];
                
                //get root joint of skeleton
                XMLElement* skeleton = instance_controller->FirstChildElement("skeleton");
                if (skeleton) {
                    std::string target_id = trim_white_space(skeleton->GetText());
                    target_id = target_id.erase(0,1);
                    entID_jointID[new_entity] = target_id;
                }
                
                //add vertex weights
                auto& opengl_weights = geomID_vertexWeights[geometries[geomID]];
                auto& opengl_jointid_strings = geomID_vertexJointIDs[geometries[geomID]];
                
                //convert joint string ids to chain ids
                std::vector<lm::ivec4> opengl_jointids(opengl_jointid_strings.size(), lm::ivec4(0,0,0,0));
                for (size_t i = 0; i < opengl_jointid_strings.size(); i++) {
                    for (size_t j = 0; j < 4; j++) {
                        std::string curr_joint_name = opengl_jointid_strings[i][j];
                        if (curr_joint_name== "")
                            continue;
                        opengl_jointids[i].value_[j] = jointName_jointPointer[curr_joint_name]->index_in_chain;
                    }
                }
                //get the geometry that was parsed and created previously
                Geometry& skin_geom = graphics_system.getGeometry(new_mesh.geometry);
                //add vertex weights to this geometry
                skin_geom.addVertexWeights(opengl_weights, opengl_jointids);
                
                //bind material
                bind_material_element = instance_controller->FirstChildElement("bind_material");
            }
            
            //geometry i.e. without skin
            //simply create new mesh using linked geometry
            XMLElement* instance_geom_element = child->FirstChildElement("instance_geometry");
            if (instance_geom_element) {
                std::string url = instance_geom_element->Attribute("url");
                url = url.erase(0, 1);
                
                Mesh& new_mesh = ECS.createComponentForEntity<Mesh>(new_entity);
                new_mesh.geometry = geometries[url];
                
                bind_material_element = instance_geom_element->FirstChildElement("bind_material");
                
            }
            
            //now we add the material to either the mesh or skinned mesh
            if (bind_material_element) {
                XMLElement* tech_common_element = bind_material_element->FirstChildElement("technique_common");
                if (tech_common_element) {
                    XMLElement* instance_material = tech_common_element->FirstChildElement("instance_material");
                    std::string target_attr = instance_material->Attribute("target");
                    target_attr = target_attr.erase(0, 1);
                    
                    bool material_failed = false;
                    if (materialID_effectID.count(target_attr) > 0) {
                        std::string effectID = materialID_effectID[target_attr];
                        
                        if (materials.count(effectID) > 0) {
                            //depending on whether we have a mesh or a skinned mesh
                            if (instance_geom_element)
                                ECS.getComponentFromEntity<Mesh>(new_entity).material = materials[effectID];
                            if (instance_controller)
                                ECS.getComponentFromEntity<SkinnedMesh>(new_entity).material = materials[effectID];
                            
                            graphics_system.getMaterial(materials[effectID]).shader_id = shader->program;
                            
                        }
                        else material_failed = true;
                    }
                    else material_failed = true;
                    
                    if (material_failed) {
                        print(("ERROR: couldn't find the material for node " + name));
                    }
                }
            }
            
        }
    }
    
    //** ASSIGN ROOT JOINT TO SKINNED MESH **/
    //Finally, once looped all nodes, we link any skinned mesh to it's root joint
    
    //loop all entities that have a <skeleton> node
    for (auto ent_joint_pair : entID_jointID) {
        //they should all have a SkinnedMesh component already!
        if (ECS.getComponentID<SkinnedMesh>(ent_joint_pair.first) != -1) {
            //link root node and number of total joints in skeleton
            SkinnedMesh& sm = ECS.getComponentFromEntity<SkinnedMesh>(ent_joint_pair.first);
            sm.root = jointID_jointPointer[ent_joint_pair.second];
            sm.num_joints = (int)jointID_jointPointer.size();
        } else {
            print(("ERROR: Skeleton doesn't have a skin!"));
            return false;
        }
    }
    
    
    
    /*** LIBRARY ANIMATIONS ***/
    
    //if no animation, carry on
    XMLElement* lib_anims = root->FirstChildElement("library_animations");
    if (!lib_anims) { print("Collada without animation parse okay"); return true; }
    
    std::unordered_map<std::string, std::vector<float>> jointname_keyframes;
    
    //for each animation
    XMLElement* anim = lib_anims->FirstChildElement("animation");
    for (anim; anim != NULL; anim = anim->NextSiblingElement("animation")) {
        //loop all sources
        
        //store source in map, identified by id string
        std::unordered_map<std::string, std::vector<float>> sources;
        
        XMLElement* source = anim->FirstChildElement("source");
        for (source; source != NULL; source = source->NextSiblingElement("source")) {
            //source id, will be key for our map
            std::string source_id = source->Attribute("id");
            
            //value is array of floats, let's read them
            std::vector<float> float_array;
            XMLElement* float_array_element = source->FirstChildElement("float_array");
            
            //skip if this source does not contain a float array
            if (!float_array_element) continue;
            
            //get the array of floats
            std::string float_array_text = trim_white_space(float_array_element->GetText());
            //split the string and set it to the float_array
            str_split_to_floats(float_array_text, " ", float_array);
            
            //finall, add to map
            sources[source_id] = float_array;
            
            
        }
        
        
        //sampler map maps the sampler ID to the output source ID (which has the float array)
        
        std::unordered_map<std::string, std::string> samplerID_outputsourceID;
        
        XMLElement* sampler = anim->FirstChildElement("sampler");
        for (sampler; sampler != NULL; sampler = sampler->NextSiblingElement("sampler")) {
            
            std::string sampler_id = sampler->Attribute("id");
            
            //we only are interested in outputs for now
            std::string output_source_id = "";
            XMLElement* input = sampler->FirstChildElement("input");
            for (input; input != NULL; input = input->NextSiblingElement("input")) {
                std::string semantic = input->Attribute("semantic");
                if (semantic == "OUTPUT") {
                    output_source_id = input->Attribute("source");
                    output_source_id = output_source_id.erase(0,1);
                }
            }
            //if we found an output source id, link it to the sampler id
            if (output_source_id != "")
                samplerID_outputsourceID[sampler_id] = output_source_id;
            
        }
        
        
        
        
        //get channels - map joint ID to sampler ID
        
        std::unordered_map<std::string, std::string> jointID_samplerID;
        
        XMLElement* channel = anim->FirstChildElement("channel");
        if (!channel) { print("ERROR: collada animation node does not have channel node"); return false; }
        
        std::string sampler_attr = channel->Attribute("source");
        sampler_attr = sampler_attr.erase(0,1);
        std::vector<std::string> target_attr;
        split(channel->Attribute("target"), "/", target_attr);
        
        jointID_samplerID[target_attr[0]] = sampler_attr;
        
        
        std::string joint_target_ID = target_attr[0];
        std::string sampler_ID = sampler_attr;
        std::string source_input_ID = samplerID_outputsourceID[sampler_ID];
        std::vector<float>& all_keyframes = sources[ source_input_ID ];
        if (all_keyframes.size() % 16 != 0) {
            std::string error_msg = "ERROR: keyframes for joint " + joint_target_ID + " are not valid!!";
            print(error_msg);
            return false;
        }
        jointID_jointPointer[joint_target_ID]->setKeyFrames(all_keyframes);
        
        
        
    }
    
    print("Collada + animation parse okay");
    return true;
}
