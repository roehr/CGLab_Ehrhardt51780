#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "GeometryNode.hpp"
#include "PointLightNode.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"
#include "texture_loader.hpp"
#include "SceneGraph.hpp"
#include <glbinding/gl/gl.h>
// use gl definitions from glbinding 
using namespace gl;

//dont load gl bindings from glfw
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>
#include <iostream>

static void update_shader_programs(std::map<std::string, shader_program>& shaders, bool throwing);
ApplicationSolar::ApplicationSolar(std::string const& resource_path)
	:Application{ resource_path }
	, planet_object{}
	, star_object{}
	, orbit_object{}
	, skybox_object{}
	, screenquad_object{}
	, m_view_transform{ glm::fmat4{} }
	, m_view_projection{ utils::calculate_projection_matrix(initial_aspect_ratio) }
{ initializeGeometry();
initializefbo();
initializeSceneGraph();
initializeShaderPrograms();
}

ApplicationSolar::~ApplicationSolar() {
	glDeleteBuffers(1, &planet_object.vertex_BO);
	glDeleteBuffers(1, &planet_object.element_BO);
	glDeleteVertexArrays(1, &planet_object.vertex_AO);
}

void ApplicationSolar::renderSceneGraph(Node* currNode) const {

	glm::fmat4 model_matrix = glm::fmat4{};
	//set a rotation value 
	//if the object is supposed to rotate by time, the function will update the matrix
	currNode->updateRotationY(0.001f);
	if (currNode != SceneGraph::getInstance().getRoot()) {
		//if we are not at the root, multiply
		currNode->setWorldTransform(currNode->getParent()->getWorldTransform()*currNode->getLocalTransform());
	}
	else {
		//root: just use LocalTransform
		currNode->setWorldTransform(currNode->getLocalTransform());
	}


	if (currNode->getType() == "PointLight") {

		//Handle lightvalues
		PointLightNode* lightNode = static_cast<PointLightNode*>(currNode);
		//pass light Information to Shaders
		glUseProgram(m_shaders.at("planet").handle);
		//position
		glUniform3f(m_shaders.at("planet").u_locs.at("LightPosition"), lightNode->getLightPosition().x, lightNode->getLightPosition().y, lightNode->getLightPosition().z);
		//color
		glUniform3f(m_shaders.at("planet").u_locs.at("LightColor"), lightNode->getLightColor().x, lightNode->getLightColor().y, lightNode->getLightColor().z);

		//intensity
		glUniform1f(m_shaders.at("planet").u_locs.at("LightIntensity"), lightNode->getLightIntensity());

	}
	//if geometry was found, render it
	if (currNode->getType() == "Geometry") {

		model_matrix = currNode->getWorldTransform();
		GeometryNode* geoNode = static_cast<GeometryNode*>(currNode);
		if (geoNode->useStar()) {
			glUseProgram(m_shaders.at("vao").handle);
			glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ModelMatrix"),
				1, GL_FALSE, glm::value_ptr(model_matrix));


			//bind starVAO
			glBindVertexArray(star_object.vertex_AO);

			// draw bound vertex array using bound shader
			glDrawArrays(GL_POINTS, 0, star_object.num_elements);


		}
		else {
			if (geoNode->useSkybox()) {
				//Shader:
				glUseProgram(m_shaders.at("skybox").handle);

				glUniformMatrix4fv(m_shaders.at("skybox").u_locs.at("ModelMatrix"),
					1, GL_FALSE, glm::value_ptr(model_matrix));


				// bind the VAO to draw
				glBindVertexArray(skybox_object.vertex_AO);


				glActiveTexture(GL_TEXTURE0);
				//bind as Cubemap
				glBindTexture(GL_TEXTURE_CUBE_MAP, geoNode->getDiffTex());
				glUniform1i(glGetUniformLocation(m_shaders.at("skybox").handle, "CubeMap"), 0);
				// draw bound vertex array using bound shader
				glDrawArrays(skybox_object.draw_mode, 0, skybox_object.num_elements);
			}
			else {
				if (geoNode->useOrbit()) {
					//all these ifs are really ugly as stated in the comment in the GeometryNode.hpp
					glUseProgram(m_shaders.at("vao").handle);
					//I've adjusted the vertexshader here so it gets the same values as the simple shader does
					//Surely I could also pass the Modelview here, but I think it is better to keep everything even since this surely is not the last shader we add

					glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ModelMatrix"),
						1, GL_FALSE, glm::value_ptr(model_matrix));


					//bind orbitVAO
					glBindVertexArray(orbit_object.vertex_AO);
					glDrawArrays(orbit_object.draw_mode, 0, orbitresolution);
				}
				else {
					glUseProgram(m_shaders.at("planet").handle);
					glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ModelMatrix"),
						1, GL_FALSE, glm::value_ptr(model_matrix));
					//handle sun seperately in shader

					if (geoNode->useSun()) {
						glUniform1i(m_shaders.at("planet").u_locs.at("Sun"), 1);
					}
					else {
						glUniform1i(m_shaders.at("planet").u_locs.at("Sun"), 0);
					}
					//color of the Planet
					glUniform3f(m_shaders.at("planet").u_locs.at("PlanetColor"), geoNode->getColor().x, geoNode->getColor().y, geoNode->getColor().z);
					// extra matrix for normal transformation to keep them orthogonal to surface
					glm::fmat4 normal_matrix = glm::inverseTranspose(glm::inverse(SceneGraph::getInstance().getActiveCamera()->getWorldTransform()) * model_matrix);
					glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("NormalMatrix"),
						1, GL_FALSE, glm::value_ptr(normal_matrix));


					//pass the maps
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, geoNode->getDiffTex());
					glUniform1i(glGetUniformLocation(m_shaders.at("planet").handle, "DiffMap"), 0);


					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, geoNode->getNormTex());
					glUniform1i(glGetUniformLocation(m_shaders.at("planet").handle, "NormMap"), 1);

					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, geoNode->getSpecTex());
					glUniform1i(glGetUniformLocation(m_shaders.at("planet").handle, "SpecMap"), 2);

					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, geoNode->getDispTex());
					glUniform1i(glGetUniformLocation(m_shaders.at("planet").handle, "DispMap"), 3);

					// bind the VAO to draw
					glBindVertexArray(planet_object.vertex_AO);

					// draw bound vertex array using bound shader
					glDrawElements(planet_object.draw_mode, planet_object.num_elements, model::INDEX.type, NULL);

				}

			}
		}

	}
	//traverse the Graph recursively 
	if (!currNode->getChildrenList().empty()) {
		std::vector<Node*>::iterator it;
		std::vector<Node*>children = currNode->getChildrenList();

		for (it = children.begin(); it != children.end(); it++) {
			renderSceneGraph(*it);
		}
	}
}

void ApplicationSolar::render() const {
	//conveniencehelper for debugging, Press O to switch
	if (offScreen) {
		//Render to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	
		//clear everythings before we go -> this avoids garbage which could influence the results
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	//always enable and disable to avoid issues when switching between on and offscreen rendering
	glEnable(GL_DEPTH_TEST);
	if (SceneGraph::getInstance().getRoot() != nullptr) {
		renderSceneGraph(SceneGraph::getInstance().getRoot());
	}
	glDisable(GL_DEPTH_TEST);
	if (offScreen) {
		//now render to screen - nothing fancy here, same procedure as always:
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindVertexArray(screenquad_object.vertex_AO);
		
		glUseProgram(m_shaders.at("screen").handle);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorbuffer);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	
	}
}

void ApplicationSolar::uploadView() {
	int width, height;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
	glViewport(0, 0, width, height);
	//this needs cleanup before it ends up with tons of shaders that need the same "treatment"
	//bind shader to which to upload unforms
	glUseProgram(m_shaders.at("planet").handle);
	// vertices are transformed in camera space, so camera transform must be inverted
	glm::fmat4 view_matrix = glm::inverse(SceneGraph::getInstance().getActiveCamera()->getWorldTransform());

	 //use our uniform block for update
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_handle);
	//target - offset -size - data - View is 2nd attribute so it needs to be offset
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view_matrix));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



void ApplicationSolar::uploadProjection() {
	//use uniformblock for update
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_handle);
	//target - offset -size - data
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(SceneGraph::getInstance().getActiveCamera()->getProjectionMatrix()));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	int width, height;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
	glViewport(0, 0, width, height);
	glBindTexture(GL_TEXTURE_2D, colorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

}

// update uniform locations
void ApplicationSolar::uploadUniforms() {

	// upload uniform values to new locations
	uploadView();
	uploadProjection();



}

///////////////////////////// intialisation functions /////////////////////////
//Random Lights
void ApplicationSolar::initializeRandomLights() {
	struct Light {
		glm::fvec3 lColor;
		glm::fvec3 lPos;
		float lInt;
	};

	int randomsize= rand() % 10 + 1;
	if (randomsize < 5){
		randomsize = 5;
	}
	
	Light* lights = new Light[randomsize];
	for (int i = 0; i < randomsize; i++) {
		lights[i].lColor = glm::fvec3((float(rand()) / RAND_MAX), (float(rand()) / RAND_MAX), (float(rand()) / RAND_MAX));
		//Positions: -1/1 multiplied by 50
		lights[i].lPos = glm::fvec3(10.0f * (float(rand()) / RAND_MAX)*2.0f - 1.0f, 10.0f * (float(rand()) / RAND_MAX)*2.0f - 1.0f, 10.0f * (float(rand()) / RAND_MAX)*2.0f - 1.0f);

		lights[i].lInt = float(rand()) / RAND_MAX;
	
	}
	std::cout << randomsize;

	//sbo

	glGenBuffers(1, &sbo_handle);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo_handle);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(lights), &lights[0], GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	//glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sbo_handle, 0, sizeof(lights));
	//glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(lights), &lights[0], GL_DYNAMIC_COPY);

	GLuint loc = glGetProgramResourceIndex(m_shaders.at("planet").handle, GL_SHADER_STORAGE_BLOCK, "LightBlock");
	glShaderStorageBlockBinding(m_shaders.at("planet").handle, loc, 0);



}
// load shader sources
void ApplicationSolar::initializeShaderPrograms() {
	// store shader program objects in container
	m_shaders.emplace("planet", shader_program{ {{GL_VERTEX_SHADER,m_resource_path + "shaders/simple.vert"},
											 {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}} });
	// request uniform locations for shader program
	m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
	m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
	//ambient color of planets
	m_shaders.at("planet").u_locs["PlanetColor"] = -1;
	m_shaders.at("planet").u_locs["LightPosition"] = -1;
	m_shaders.at("planet").u_locs["LightColor"] = -1;
	m_shaders.at("planet").u_locs["LightIntensity"] = -1;
	m_shaders.at("planet").u_locs["Toon"] = -1;
	m_shaders.at("planet").u_locs["Sun"] = -1;
	m_shaders.at("planet").u_locs["MultiLight"] = -1;
	//next Shader
	m_shaders.emplace("vao", shader_program{ {{GL_VERTEX_SHADER,m_resource_path + "shaders/vao.vert"},
											{GL_FRAGMENT_SHADER, m_resource_path + "shaders/vao.frag"}} });
	// request uniform locations for shader program
	m_shaders.at("vao").u_locs["ModelMatrix"] = -1;



	//next Shader
	m_shaders.emplace("skybox", shader_program{ {{GL_VERTEX_SHADER,m_resource_path + "shaders/skybox.vert"},
											{GL_FRAGMENT_SHADER, m_resource_path + "shaders/skybox.frag"}} });
	// request uniform locations for shader program
	m_shaders.at("skybox").u_locs["ModelMatrix"] = -1;


	//next Shader
	m_shaders.emplace("screen", shader_program{ {{GL_VERTEX_SHADER,m_resource_path + "shaders/screen.vert"},
											{GL_FRAGMENT_SHADER, m_resource_path + "shaders/screen.frag"}} });
	m_shaders.at("screen").u_locs["Grey"] = -1;
	m_shaders.at("screen").u_locs["MirrorX"] = -1;
	m_shaders.at("screen").u_locs["MirrorY"] = -1;
	m_shaders.at("screen").u_locs["Gauss"] = -1;
	//very dirty -> duplicate Code! 
	//I have one issue here: 
	//before we can initialize the Uniform Block, the shaders need to be linked at least ONCE
	//in the application.cpp this is done by calling reloadShaders and that function can be called from here
	//BUT: it also calls the update functions - and those need to have the initialized Uniform Block to work
	//so this is one (not so nice) way to work around it
	update_shader_programs(m_shaders,true);
	initializeubo();
}

void ApplicationSolar::initializeubo() {

	//ubo
	//Define a location for all shaders which will use it 
	GLuint locpl = glGetUniformBlockIndex(m_shaders.at("planet").handle, "CameraBlock");
	glUniformBlockBinding(m_shaders.at("planet").handle, locpl, 0);
	GLuint locvao = glGetUniformBlockIndex(m_shaders.at("vao").handle, "CameraBlock");
	glUniformBlockBinding(m_shaders.at("vao").handle, locvao, 0);
	GLuint locsky = glGetUniformBlockIndex(m_shaders.at("skybox").handle, "CameraBlock");
	glUniformBlockBinding(m_shaders.at("skybox").handle, locsky, 0);
	//we know that by now...
	glGenBuffers(1, &ubo_handle);
	//I couldn't get this to work as provided on the sheets - so I chose an alternate way:
	//nothing new here
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_handle);
	//This will be our Buffer layout: 2 Mat4 Matrices
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	//I define the buffer to use the whole range - we will subdivide it using SubData
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo_handle, 0, 2 * sizeof(glm::mat4));

}





void ApplicationSolar::initializeSkyboxTextures(GeometryNode* skybox) {
	//define the textures
	std::vector<std::string> textures
	{
		m_resource_path + "textures/galaxy+X.png",
		m_resource_path + "textures/galaxy-X.png",
		m_resource_path + "textures/galaxy+Y.png",
		m_resource_path + "textures/galaxy-Y.png",
		m_resource_path + "textures/galaxy+Z.png",
		m_resource_path + "textures/galaxy-Z.png"
	};


	//prepare
	glGenTextures(1, skybox->getDiffTexptr());
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->getDiffTex());
	//glBindTexture(GL_TEXTURE_2D, skybox->getDiffTex());
	//pixel_data colortex = texture_loader::file(textures[0]);
   // glTexImage2D(GL_TEXTURE_2D, 0, (GLint)colortex.channels, colortex.width, colortex.height, 0, colortex.channels, colortex.channel_type, colortex.ptr());
   //assign the textures
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		pixel_data colortex = texture_loader::file(textures[i]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, (GLint)colortex.channels, colortex.width, colortex.height, 0, colortex.channels, colortex.channel_type, colortex.ptr());

	}
	//setup parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

//initialize SceneGraph
void ApplicationSolar::initializeSceneGraph() {

	model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL);
	//initializing the SceneGraph - every Matrix is a single transformation step 
	//all planet values are completly random
	//note on the camera: as of now it is handled as part of the scenegraph though the updating is left as it was
	//that means: if the camera is set as a child of a planet/moon or something else moving in the scene, the next processed
	//key or mousemovement will be updated relative to the movement of that object
	//this is intentional because it allows to easily track an object by just putting the camera update within the render procedure
	//as long as the camera is a child of root, it will work like in the example which was provided 
	Node* root = SceneGraph::getInstance().getRoot();

	//Skybox:
	GeometryNode* skybox = new GeometryNode("Skybox");
	skybox->setColor(glm::fvec3{ 0.5f, 0.5f, 0.5f });
	skybox->setLocalTransform(glm::scale(glm::fmat4{}, glm::vec3{ 20.0f,20.0f,20.0f }));
	skybox->setSkybox(true);
	root->addChildren(skybox);
	skybox->setParent(root);
	initializeSkyboxTextures(skybox);

	//add the lightsource to our scene

	PointLightNode * light = new PointLightNode("Sunlight");

	light->setLightIntensity(1.0f);
	light->setLightColor(glm::fvec3{ 1.0f, 1.0f, 1.0f });
	light->setParent(root);
	light->setLightPosition(glm::fvec3{ 0.0f,0.0f,0.0f });
	root->addChildren(light);
	light->setLocalTransform(glm::translate(glm::fmat4{}, light->getLightPosition()));

	GeometryNode* sun = new GeometryNode("Sun");
	sun->setColor(glm::fvec3{ 1.0f,1.0f,0.0f });
	sun->setLocalTransform(glm::fmat4{});
	sun->setModel(planet_model);
	sun->changeTimeDependency();
	sun->setSun(true);
	//This should be optimized: once either a root is set or a child is added, the corresponding function should be set automatically
	root->addChildren(sun);
	sun->setParent(root);
	initializePlanetTextures(sun, "sunmap.png", "sunnormal.png", "sunspecular.png", "sundisplacement.png");
	//mercury
	//one node handles rotationspeed around sun
	//Since in this setup root resembles the position of the sun, this is also the parent of the planets
	Node* planetaroundsunrotation = new Node("Mercury Sunrotation");

	planetaroundsunrotation->setLocalTransform(glm::fmat4{});
	//control speed in which the planet rotates around the sun
	planetaroundsunrotation->changeTimeDependency();
	planetaroundsunrotation->setRotationY(0.5f);

	planetaroundsunrotation->setParent(root);
	root->addChildren(planetaroundsunrotation);

	//translation -> basically the orbit, distance planet to sun 
	Node* planettranslation = new Node("Mercury Translate");
	//GeometryNode: This is the planet as it will be rendered - it also holds the values for rotation around itself
	GeometryNode* planet = new GeometryNode("Mercury Rotation");

	planetaroundsunrotation->addChildren(planettranslation);
	planettranslation->setParent(planetaroundsunrotation);
	glm::fmat4 planettranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -2.0f });
	planettranslation->setLocalTransform(planettranslationmatrix);
	//Orbit: basically a circle scaled up to the distance between sun and planet
	GeometryNode* planetOrbit = new GeometryNode("Mercury Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(root);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -2.0f, -2.0f, -2.0f }));

	planet->setModel(planet_model);
	glm::fmat4 	planetmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	planetmatrix = glm::scale(planetmatrix, glm::fvec3{ 0.3f, 0.3f, 0.3f });
	planet->setLocalTransform(planetmatrix);
	planet->setRotationY(0.1);
	planet->changeTimeDependency();
	planettranslation->addChildren(planet);
	planet->setParent(planettranslation);
	planet->setColor(glm::fvec3{ 0.5f,0.0f,0.0f });
	initializePlanetTextures(planet, "mercurymap.png", "mercurynormal.png", "mercuryspecular.png", "mercurydisplacement.png");

	//Venus

	planetaroundsunrotation = new Node("Venus Sunrotation");

	planetaroundsunrotation->setLocalTransform(glm::fmat4{});
	//control speed in which the planet rotates around the sun
	planetaroundsunrotation->changeTimeDependency();
	planetaroundsunrotation->setRotationY(0.3f);

	planetaroundsunrotation->setParent(root);
	root->addChildren(planetaroundsunrotation);
	planetOrbit = new GeometryNode("Venus Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(root);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -3.0f, -3.0f, -3.0f }));

	planettranslation = new Node("Venus Translate");
	planet = new GeometryNode("Venus Rotation");
	planettranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -3.0f });
	planettranslation->setLocalTransform(planettranslationmatrix);
	planettranslation->changeTimeDependency();
	planettranslation->setRotationY(0.5f);
	planetaroundsunrotation->addChildren(planettranslation);
	planettranslation->setParent(planetaroundsunrotation);


	planet->setModel(planet_model);
	planetmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	planetmatrix = glm::scale(planetmatrix, glm::fvec3{ 0.4f, 0.4f, 0.4f });
	planet->setLocalTransform(planetmatrix);
	planet->setRotationY(0.2);
	planet->changeTimeDependency();
	planettranslation->addChildren(planet);
	planet->setParent(planettranslation);
	planet->setColor(glm::fvec3{ 0.0f,0.5f,0.0f });

	initializePlanetTextures(planet, "venusmap.png", "venusnormal.png", "venusspecular.png", "venusdisplacement.png");

	//Earth
	planetaroundsunrotation = new Node("Earth Sunrotation");

	planetaroundsunrotation->setLocalTransform(glm::fmat4{});
	//control speed in which the planet rotates around the sun
	planetaroundsunrotation->changeTimeDependency();
	planetaroundsunrotation->setRotationY(0.5f);

	planetaroundsunrotation->setParent(root);
	root->addChildren(planetaroundsunrotation);
	planetOrbit = new GeometryNode("Earth Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(root);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -6.0f, -6.0f, -6.0f }));

	planettranslation = new Node("Earth Translate");
	planettranslation->changeTimeDependency();
	planettranslation->setRotationY(0.6f);


	planet = new GeometryNode("Earth Rotation");
	planettranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -6.0f });
	planettranslation->setLocalTransform(planettranslationmatrix);
	planetaroundsunrotation->addChildren(planettranslation);
	planettranslation->setParent(planetaroundsunrotation);


	planet->setModel(planet_model);
	planetmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	planetmatrix = glm::scale(planetmatrix, glm::fvec3{ 0.6f, 0.6f, 0.6f });
	planet->setLocalTransform(planetmatrix);
	planet->setRotationY(0.4);
	planet->changeTimeDependency();
	planettranslation->addChildren(planet);
	planet->setParent(planettranslation);
	planet->setColor(glm::fvec3{ 0.0f,0.0f,0.5f });
	initializePlanetTextures(planet, "earthmap.png", "earthnormal.png", "earthspecular.png", "testdisplacement.png");


	//moon
	//basically transformations work as with a planet, just that in this case the earth will be our center of rotation
	Node* moontranslation = new Node("Moon Translate");
	GeometryNode* moon = new GeometryNode("Moon Rotation");
	planet->addChildren(moontranslation);
	moontranslation->setParent(planet);
	moontranslation->changeTimeDependency();
	moontranslation->setRotationY(0.2f);
	glm::fmat4 moontranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -1.5f });
	moontranslation->setLocalTransform(moontranslationmatrix);


	planetOrbit = new GeometryNode("Moon Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(planet);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -1.5f, -1.5f, -1.5f }));


	moon->setModel(planet_model);
	glm::fmat4 	moonmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	moonmatrix = glm::scale(moonmatrix, glm::fvec3{ 0.1f, 0.1f, 0.1f });
	moon->setLocalTransform(moonmatrix);
	moon->setRotationY(0.1);
	moon->changeTimeDependency();
	moontranslation->addChildren(moon);
	moon->setParent(moontranslation);
	moon->setColor(glm::fvec3{ 0.5f,0.5f,0.5f });
	initializePlanetTextures(moon, "moonmap.png", "moonnormal.png", "moonspecular.png", "moondisplacement.png");


	//Mars
	planetaroundsunrotation = new Node("Mars Sunrotation");

	planetaroundsunrotation->setLocalTransform(glm::fmat4{});
	//control speed in which the planet rotates around the sun
	planetaroundsunrotation->changeTimeDependency();
	planetaroundsunrotation->setRotationY(0.8f);

	planetaroundsunrotation->setParent(root);
	root->addChildren(planetaroundsunrotation);


	planetOrbit = new GeometryNode("Mars Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(root);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -9.0f, -9.0f, -9.0f }));

	planettranslation = new Node("Mars Translate");
	planet = new GeometryNode("Mars Rotation");
	planettranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -9.0f });
	planettranslation->setLocalTransform(planettranslationmatrix);
	planetaroundsunrotation->addChildren(planettranslation);
	planettranslation->setParent(planetaroundsunrotation);


	planet->setModel(planet_model);
	planetmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	planetmatrix = glm::scale(planetmatrix, glm::fvec3{ 0.7f, 0.7f, 0.7f });
	planet->setLocalTransform(planetmatrix);
	planet->setRotationY(0.4);
	planet->changeTimeDependency();
	planettranslation->addChildren(planet);
	planet->setParent(planettranslation);
	planet->setColor(glm::fvec3{ 0.5f,0.25f,0.25f });
	initializePlanetTextures(planet, "marsmap.png", "marsnormal.png", "marsspecular.png", "marsdisplacement.png");

	//Jupiter
	planetaroundsunrotation = new Node("Jupiter Sunrotation");

	planetaroundsunrotation->setLocalTransform(glm::fmat4{});
	//control speed in which the planet rotates around the sun
	planetaroundsunrotation->changeTimeDependency();
	planetaroundsunrotation->setRotationY(0.4f);

	planetOrbit = new GeometryNode("Jupiter Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(root);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -12.0f, -12.0f, -12.0f }));

	planetaroundsunrotation->setParent(root);
	root->addChildren(planetaroundsunrotation);
	planettranslation = new Node("Jupiter Translate");
	planet = new GeometryNode("Jupiter Rotation");
	planettranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -12.0f });
	planettranslation->setLocalTransform(planettranslationmatrix);
	planetaroundsunrotation->addChildren(planettranslation);
	planettranslation->setParent(planetaroundsunrotation);


	planet->setModel(planet_model);
	planetmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	planetmatrix = glm::scale(planetmatrix, glm::fvec3{ 0.7f, 0.7f, 0.7f });
	planet->setLocalTransform(planetmatrix);
	planet->setRotationY(0.4);
	planet->changeTimeDependency();
	planettranslation->addChildren(planet);
	planet->setParent(planettranslation);
	planet->setColor(glm::fvec3{ 0.5f,0.5f,0.25f });

	initializePlanetTextures(planet, "jupitermap.png", "jupiternormal.png", "jupiterspecular.png", "jupiterdisplacement.png");


	//Saturn
	planetaroundsunrotation = new Node("Saturn Sunrotation");

	planetaroundsunrotation->setLocalTransform(glm::fmat4{});
	//control speed in which the planet rotates around the sun
	planetaroundsunrotation->changeTimeDependency();
	planetaroundsunrotation->setRotationY(0.5f);

	planetaroundsunrotation->setParent(root);
	root->addChildren(planetaroundsunrotation);

	planetOrbit = new GeometryNode("Saturn Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(root);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -15.0f, -15.0f, -15.0f }));

	planettranslation = new Node("Saturn Translate");
	planet = new GeometryNode("Saturn Rotation");
	planettranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -15.0f });
	planettranslation->setLocalTransform(planettranslationmatrix);
	planetaroundsunrotation->addChildren(planettranslation);
	planettranslation->setParent(planetaroundsunrotation);


	planet->setModel(planet_model);
	planetmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	planetmatrix = glm::scale(planetmatrix, glm::fvec3{ 0.8f, 0.8f, 0.8f });
	planet->setLocalTransform(planetmatrix);
	planet->setRotationY(0.6);
	planet->changeTimeDependency();
	planettranslation->addChildren(planet);
	planet->setParent(planettranslation);
	planet->setColor(glm::fvec3{ 0.25f,0.25f,0.5f });

	initializePlanetTextures(planet, "saturnmap.png", "saturnnormal.png", "saturnspecular.png", "saturndisplacement.png");


	//Uranus
	planetaroundsunrotation = new Node("Uranus Sunrotation");

	planetaroundsunrotation->setLocalTransform(glm::fmat4{});
	//control speed in which the planet rotates around the sun
	planetaroundsunrotation->changeTimeDependency();
	planetaroundsunrotation->setRotationY(0.3f);

	planetaroundsunrotation->setParent(root);
	root->addChildren(planetaroundsunrotation);

	planetOrbit = new GeometryNode("Uranus Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(root);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -17.0f, -17.0f, -17.0f }));

	planettranslation = new Node("Uranus Translate");
	planet = new GeometryNode("Uranus Rotation");
	planettranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -17.0f });
	planettranslation->setLocalTransform(planettranslationmatrix);
	planetaroundsunrotation->addChildren(planettranslation);
	planettranslation->setParent(planetaroundsunrotation);


	planet->setModel(planet_model);
	planetmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	planetmatrix = glm::scale(planetmatrix, glm::fvec3{ 0.9f, 0.9f, 0.9f });
	planet->setLocalTransform(planetmatrix);
	planet->setRotationY(0.6);
	planet->changeTimeDependency();
	planettranslation->addChildren(planet);
	planet->setParent(planettranslation);

	planet->setColor(glm::fvec3{ 0.0f,0.2f,0.5f });
	initializePlanetTextures(planet, "uranusmap.png", "uranusnormal.png", "uranusspecular.png", "uranusdisplacement.png");


	//Neptune

	planetaroundsunrotation = new Node("Neptune Sunrotation");
	planetaroundsunrotation->setLocalTransform(glm::fmat4{});
	//control speed in which the planet rotates around the sun
	planetaroundsunrotation->changeTimeDependency();
	planetaroundsunrotation->setRotationY(0.1f);

	planetaroundsunrotation->setParent(root);
	root->addChildren(planetaroundsunrotation);
	planetOrbit = new GeometryNode("Neptune Orbit");
	planetOrbit->setOrbit(true);
	planetOrbit->setParent(root);
	root->addChildren(planetOrbit);
	planetOrbit->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ -20.0f, -20.0f, -20.0f }));
	planettranslation = new Node("Neptune Translate");
	planet = new GeometryNode("Neptune Rotation");
	planettranslationmatrix = glm::translate(glm::fmat4{}, glm::fvec3{ 0.0f, 0.0f, -20.0f });
	planettranslation->setLocalTransform(planettranslationmatrix);
	planetaroundsunrotation->addChildren(planettranslation);
	planettranslation->setParent(planetaroundsunrotation);


	planet->setModel(planet_model);
	planetmatrix = glm::rotate(glm::fmat4{}, 1.0f, glm::fvec3{ 0.0f, 1.0f, 0.0f });
	planetmatrix = glm::scale(planetmatrix, glm::fvec3{ 0.9f, 0.9f, 0.9f });
	planet->setLocalTransform(planetmatrix);
	planet->setRotationY(0.6);
	planet->changeTimeDependency();
	planettranslation->addChildren(planet);
	planet->setParent(planettranslation);
	planet->setColor(glm::fvec3{ 0.5f,0.3f,0.5f });
	initializePlanetTextures(planet, "neptunemap.png", "neptunenormal.png", "neptunespecular.png", "neptunedisplacement.png");

	CameraNode* cam = new CameraNode("Camera");
	cam->setParent(root);
	root->addChildren(cam);
	cam->setEnabled(true);
	cam->setProjectionMatrix(m_view_projection);
	cam->setCamPosition(glm::fvec3{ 0.0f, 0.0f, 4.0f });
	cam->setLocalTransform(glm::translate(m_view_transform, cam->getCamPosition()));
	cam->setWorldTransform(glm::translate(m_view_transform, cam->getCamPosition()));

	GeometryNode* stars = new GeometryNode("Stars");
	stars->setParent(root);
	stars->setStar(true);
	root->addChildren(stars);
	stars->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ 20.0f, 20.0f, 20.0f }));
	std::cout << SceneGraph::getInstance().printGraph();
}



//load textures
void ApplicationSolar::initializePlanetTextures(GeometryNode* planet, std::string diff, std::string norm, std::string spec, std::string disp) {

	//prepare
	glActiveTexture(GL_TEXTURE0);
	pixel_data colortex = texture_loader::file(m_resource_path + "textures/" + diff);
	glGenTextures(1, planet->getDiffTexptr());
	glBindTexture(GL_TEXTURE_2D, planet->getDiffTex());
	//parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
	//define texture and data format
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)colortex.channels, colortex.width, colortex.height, 0, colortex.channels, colortex.channel_type, colortex.ptr());

	//prepare
	glActiveTexture(GL_TEXTURE1);
	colortex = texture_loader::file(m_resource_path + "textures/" + norm);
	glGenTextures(1, planet->getNormTexptr());
	glBindTexture(GL_TEXTURE_2D, planet->getNormTex());
	//parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
	//define texture and data format
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)colortex.channels, colortex.width, colortex.height, 0, colortex.channels, colortex.channel_type, colortex.ptr());

	//prepare
	glActiveTexture(GL_TEXTURE2);
	colortex = texture_loader::file(m_resource_path + "textures/" + spec);
	glGenTextures(1, planet->getSpecTexptr());
	glBindTexture(GL_TEXTURE_2D, planet->getSpecTex());
	//parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
	//define texture and data format
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)colortex.channels, colortex.width, colortex.height, 0, colortex.channels, colortex.channel_type, colortex.ptr());

	//prepare
	glActiveTexture(GL_TEXTURE3);
	colortex = texture_loader::file(m_resource_path + "textures/" + disp);
	glGenTextures(1, planet->getDispTexptr());
	glBindTexture(GL_TEXTURE_2D, planet->getDispTex());
	//parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
	//define texture and data format
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)colortex.channels, colortex.width, colortex.height, 0, colortex.channels, colortex.channel_type, colortex.ptr());

}

void ApplicationSolar::initializefbo() {
	//define the quad (positions+uv), we only need xy positions, since z is always 0 
	float screenquad[] = {
		
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};
	//VAO/VBO etc. - same as always:
	glGenVertexArrays(1, &screenquad_object.vertex_AO);
	glBindVertexArray(screenquad_object.vertex_AO);
	glGenBuffers(1, &screenquad_object.vertex_BO);
	glBindBuffer(GL_ARRAY_BUFFER, screenquad_object.vertex_BO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenquad), &screenquad, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	//position first
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	//uv second
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	screenquad_object.draw_mode = GL_TRIANGLES;
	
	//now the FBO:
	//generate and bind
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	//colortexture
	glGenTextures(1, &colorbuffer);
	glBindTexture(GL_TEXTURE_2D, colorbuffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
	//use current image values as size
	int width, height;
	glfwGetWindowSize(glfwGetCurrentContext(),&width, &height);
	
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_RGBA8, width,height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	//Texture Object attachments
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,colorbuffer, 0);
	//Now for the depth
	//define buffer
	glGenRenderbuffers(1, &renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer); 
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER, renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,width, height);

	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, draw_buffers);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Error";
	}
	else {
		std::cout << "framebuffer ok!";
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


}

// load models
void ApplicationSolar::initializeGeometry() {
	model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL | model::TEXCOORD);

	// generate vertex array object
	glGenVertexArrays(1, &planet_object.vertex_AO);
	// bind the array for attaching buffers
	glBindVertexArray(planet_object.vertex_AO);

	// generate generic buffer
	glGenBuffers(1, &planet_object.vertex_BO);
	// bind this as an vertex array buffer containing all attributes
	glBindBuffer(GL_ARRAY_BUFFER, planet_object.vertex_BO);
	// configure currently bound array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * planet_model.data.size(), planet_model.data.data(), GL_STATIC_DRAW);

	// activate first attribute on gpu
	glEnableVertexAttribArray(0);
	// first attribute is 3 floats with no offset & stride
	glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::POSITION]);
	// activate second attribute on gpu
	glEnableVertexAttribArray(1);
	// second attribute is 3 floats with no offset & stride
	glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::NORMAL]);


	// texcoords:
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, model::TEXCOORD.components, model::TEXCOORD.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::TEXCOORD]);

	//For testing pass tangent and bitangent as well
	// tangent:
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, model::TANGENT.components, model::TANGENT.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::TANGENT]);

	//bitangent:
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, model::BITANGENT.components, model::BITANGENT.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::BITANGENT]);


	// generate generic buffer
	glGenBuffers(1, &planet_object.element_BO);
	// bind this as an vertex array buffer containing all attributes
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planet_object.element_BO);
	// configure currently bound array buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size * planet_model.indices.size(), planet_model.indices.data(), GL_STATIC_DRAW);

	// store type of primitive to draw
	planet_object.draw_mode = GL_TRIANGLES;
	// transfer number of indices to model object 
	planet_object.num_elements = GLsizei(planet_model.indices.size());

	//star Object:
	  //initialize some stars:
	std::vector<float>  star_model;
	unsigned starCounter = 1000;//number of stars to draw
	for (unsigned i = 0; i < starCounter; i++)
	{

		//Just some random points, adjusted to be between -1 and 1 - scaling will happen through the Scene Graph initialization
		star_model.push_back((float(rand()) / RAND_MAX)*2.0f - 1.0f);
		star_model.push_back((float(rand()) / RAND_MAX)*2.0f - 1.0f);
		star_model.push_back((float(rand()) / RAND_MAX)*2.0f - 1.0f);
		//the colors of the stars represented by  values between 0 and 1
		star_model.push_back((float(rand()) / RAND_MAX));
		star_model.push_back((float(rand()) / RAND_MAX));
		star_model.push_back((float(rand()) / RAND_MAX));


	}
	//now define a VAO
	glGenVertexArrays(1, &star_object.vertex_AO);
	glBindVertexArray(star_object.vertex_AO);
	//VBO
	glGenBuffers(1, &star_object.vertex_BO);
	glBindBuffer(GL_ARRAY_BUFFER, star_object.vertex_BO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*star_model.size(), &(star_model[0]), GL_STATIC_DRAW);
	//now give some information of the layout
	glEnableVertexAttribArray(0);
	//location 0 in Shader:
	//position, 3 values(xyz), all floats, no normalization, 6 attributesoverall, offset is 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (GLvoid*)(0));
	//now color, offset 3
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (GLvoid*)(sizeof(float) * 3));
	//Bind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	//set values needed for rendering
	star_object.draw_mode = GL_POINTS;
	star_object.num_elements = starCounter;


	//next orbit, basicliy we definy values of a circle:
	std::vector<float>  orbit_model;
	float angle = 0.0;
	//resolution defines in how many segments the circle will be divided

	while (angle < 2.0f * 3.14f) {
		//use sin and cos to define xyz values
		orbit_model.push_back(std::cos(angle));
		orbit_model.push_back(0.0f);
		orbit_model.push_back(std::sin(angle));
		//this will be the color of our orbits - just use some grey
		orbit_model.push_back(0.7f);
		orbit_model.push_back(0.7f);
		orbit_model.push_back(0.7f);

		angle += 2.0f*3.14f / orbitresolution;
	}

	//VAO
	glGenVertexArrays(1, &orbit_object.vertex_AO);
	glBindVertexArray(orbit_object.vertex_AO);
	//VBO
	glGenBuffers(1, &orbit_object.vertex_BO);
	glBindBuffer(GL_ARRAY_BUFFER, orbit_object.vertex_BO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*orbit_model.size(), &(orbit_model[0]), GL_STATIC_DRAW);
	//now layout - basically same as with the stars:
	glEnableVertexAttribArray(0);
	//location 0 for our shader - position :
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (GLvoid*)(0));
	//location 1 in Shader - color
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (GLvoid*)(sizeof(float) * 3));
	//Bind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	//set values needed for rendering
	orbit_object.draw_mode = GL_LINE_LOOP;
	orbit_object.num_elements = orbit_model.size() / 6;


	//add a skybox
	std::vector<float>  skybox_model;
	//define the vertices of a cube:
	float skyVerts[] = {
		// a cube consist of 6 sides, each side needs 2 triangles         
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,

		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,

		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,

		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,

		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	skybox_model.insert(skybox_model.end(), std::begin(skyVerts), std::end(skyVerts));
	//VAO
	glGenVertexArrays(1, &skybox_object.vertex_AO);
	glBindVertexArray(skybox_object.vertex_AO);
	//VBO
	glGenBuffers(1, &skybox_object.vertex_BO);
	glBindBuffer(GL_ARRAY_BUFFER, skybox_object.vertex_BO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*skybox_model.size(), &(skybox_model[0]), GL_STATIC_DRAW);
	//now layout - only the position is passed:
	glEnableVertexAttribArray(0);
	//location 0 for our shader - position:
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (GLvoid*)(0));
	//Bind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	//set values needed for rendering
	skybox_object.draw_mode = GL_TRIANGLES;
	skybox_object.num_elements = skybox_model.size() / 3;

}


///////////////////////////// callback functions for window events ////////////
// handle key input
void ApplicationSolar::keyCallback(int key, int action, int mods) {
	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		glm::fvec3 offset = glm::fvec3{ 0.0f, 0.0f, -0.1f };
		//  SceneGraph::getInstance().getActiveCamera()->setCamPosition(SceneGraph::getInstance().getActiveCamera()->getCamPosition() + offset);
		SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::translate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), offset));

		uploadView();
	}
	else if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) {

		glm::fvec3 offset = glm::fvec3{ 0.0f, 0.0f, 0.1f };
		//  SceneGraph::getInstance().getActiveCamera()->setCamPosition(SceneGraph::getInstance().getActiveCamera()->getCamPosition() + offset);
		SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::translate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), offset));

		uploadView();
	}
	//add a few more controls to allow for rotations:
	else if (key == GLFW_KEY_X && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::rotate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), 0.01f, glm::fvec3{ 1.0f, 0.0f, 0.0f }));
		uploadView();
	}
	else if (key == GLFW_KEY_Y && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::rotate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), 0.01f, glm::fvec3{ 0.0f, 1.0f, 0.0f }));
		uploadView();
	}
	else if (key == GLFW_KEY_Z && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::rotate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), 0.01f, glm::fvec3{ 0.0f, 0.0f, -1.0f }));

		uploadView();
	}
	//Handle Toon Shading
	else if (key == GLFW_KEY_3 && action == GLFW_PRESS) {

		glUseProgram(m_shaders.at("planet").handle);
		if (multilight) {
			multilight = false;
			glUniform1i(m_shaders.at("planet").u_locs.at("MultiLight"), 0);
		}
		else {
			multilight = true;
			initializeRandomLights();
			glUniform1i(m_shaders.at("planet").u_locs.at("MultiLight"), 1);
		}

	}
	//Handle Toon Shading
	else if (key == GLFW_KEY_2) {
		glUseProgram(m_shaders.at("planet").handle);
		glUniform1i(m_shaders.at("planet").u_locs.at("Toon"), 0);
	}
	else if (key == GLFW_KEY_1) {
		glUseProgram(m_shaders.at("planet").handle);
		glUniform1i(m_shaders.at("planet").u_locs.at("Toon"), 1);
	}
	//Additional Keys for Offscreen
	else if (key == GLFW_KEY_7 && action == GLFW_PRESS) {
		
		glUseProgram(m_shaders.at("screen").handle);
		if (grey) {
			grey = false;
			glUniform1i(m_shaders.at("screen").u_locs.at("Grey"), 0);
		}
		else {
			grey = true;
			glUniform1i(m_shaders.at("screen").u_locs.at("Grey"), 1);
		}
	
	}

	else if (key == GLFW_KEY_8 && action == GLFW_PRESS) {

		glUseProgram(m_shaders.at("screen").handle);
		if (mirrorx) {
			mirrorx = false;
			glUniform1i(m_shaders.at("screen").u_locs.at("MirrorX"), 0);
		}
		else {
			mirrorx = true;
			glUniform1i(m_shaders.at("screen").u_locs.at("MirrorX"), 1);
		}

	}
	else if (key == GLFW_KEY_9 && action == GLFW_PRESS) {

		glUseProgram(m_shaders.at("screen").handle);
		if (mirrory) {
			mirrory= false;
			glUniform1i(m_shaders.at("screen").u_locs.at("MirrorY"), 0);
		}
		else {
			mirrory = true;
			glUniform1i(m_shaders.at("screen").u_locs.at("MirrorY"), 1);
		}

	}
	else if (key == GLFW_KEY_0 && action == GLFW_PRESS) {

		glUseProgram(m_shaders.at("screen").handle);
		if (gauss) {
			gauss = false;
			glUniform1i(m_shaders.at("screen").u_locs.at("Gauss"), 0);
		}
		else {
			gauss = true;
			glUniform1i(m_shaders.at("screen").u_locs.at("Gauss"), 1);
		}

	}
	else if (key == GLFW_KEY_F && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		GLFWmonitor *mon= glfwGetPrimaryMonitor();
		if (fullScreen) {
			fullScreen = false;
			ApplicationSolar::resizeCallback(1024, 768);

		}
		else {
			fullScreen = true;
			ApplicationSolar::resizeCallback(3000, 1800);
		}
		
		uploadView();
	}
	else if (key == GLFW_KEY_O && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		if (offScreen) {
			offScreen = false;
		}
		else {
			offScreen = true;
		}
	
	}
}

//handle delta mouse movement input
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
	// mouse handling
	glm::fvec3 offset = glm::fvec3{ 0.001f*pos_x, 0.001f*pos_y, 0.0f };
	SceneGraph::getInstance().getActiveCamera()->setCamPosition(SceneGraph::getInstance().getActiveCamera()->getCamPosition() + offset);
	SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::translate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), offset));
	uploadView();
}

//handle resizing
void ApplicationSolar::resizeCallback(unsigned width, unsigned height) {
	// recalculate projection matrix for new aspect ration
	glfwSetWindowSize(glfwGetCurrentContext(),width, height);
	SceneGraph::getInstance().getActiveCamera()->setProjectionMatrix(utils::calculate_projection_matrix(float(width) / float(height)));
	// upload new projection matrix
	uploadProjection();
}


// exe entry point
int main(int argc, char* argv[]) {
	Application::run<ApplicationSolar>(argc, argv, 3, 2);
}

// update uniform locations
static void update_shader_programs(std::map<std::string, shader_program>& shaders, bool throwing) {
	// actual functionality in lambda to allow update with and without throwing
	auto update_lambda = [](shader_program& program) {
		// throws exception when compiling was unsuccessfull
		GLuint new_program = shader_loader::program(program.shader_paths);
		// free old shader program
		glDeleteProgram(program.handle);
		// save new shader program
		program.handle = new_program;
	};

	// reload all shader programs
	for (auto& pair : shaders) {
		if (throwing) {
			update_lambda(pair.second);
		}
		else {
			try {
				update_lambda(pair.second);
			}
			catch (std::exception&) {
				// dont crash, allow another try
			}
		}
	}
}