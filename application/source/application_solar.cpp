#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "GeometryNode.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"
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

ApplicationSolar::ApplicationSolar(std::string const& resource_path)
 :Application{resource_path}
 ,planet_object{}
	, star_object{}
	, orbit_object{}
 ,m_view_transform{glm::translate(glm::fmat4{}, glm::fvec3{0.0f, 0.0f, 4.0f})}
 ,m_view_projection{utils::calculate_projection_matrix(initial_aspect_ratio)}
{ initializeGeometry();

initializeShaderPrograms();
initializeSceneGraph();
  
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
	//if geometry was found, render it
	if (currNode->getType()=="Geometry") {


		 model_matrix = currNode->getWorldTransform();
		 GeometryNode* geoNode = static_cast<GeometryNode*>(currNode);
		 if(geoNode->useStar()){
			 glUseProgram(m_shaders.at("vao").handle);
			 glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ModelMatrix"),
				 1, GL_FALSE, glm::value_ptr(model_matrix));


			 //bind starVAO
			 glBindVertexArray(star_object.vertex_AO);

			 // draw bound vertex array using bound shader
			 glDrawArrays(GL_POINTS, 0, star_object.num_elements);
		

		 }
		 else{
			if(geoNode->useOrbit()){
				glUseProgram(m_shaders.at("vao").handle);
				glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ModelMatrix"),
					1, GL_FALSE, glm::value_ptr(model_matrix));


				//bind orbitVAO
				glBindVertexArray(orbit_object.vertex_AO);

		
				glDrawArrays(orbit_object.draw_mode, 0, 360.f);
			}
			else {
				glUseProgram(m_shaders.at("planet").handle);
				glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ModelMatrix"),
					1, GL_FALSE, glm::value_ptr(model_matrix));

				// extra matrix for normal transformation to keep them orthogonal to surface
				glm::fmat4 normal_matrix = glm::inverseTranspose(glm::inverse(SceneGraph::getInstance().getActiveCamera()->getWorldTransform()) * model_matrix);
				glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("NormalMatrix"),
					1, GL_FALSE, glm::value_ptr(normal_matrix));

				// bind the VAO to draw
				glBindVertexArray(planet_object.vertex_AO);

				// draw bound vertex array using bound shader
				glDrawElements(planet_object.draw_mode, planet_object.num_elements, model::INDEX.type, NULL);

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
  
  if (SceneGraph::getInstance().getRoot() != nullptr) {
	  	  //start traversing the Scenegraph
	  renderSceneGraph(SceneGraph::getInstance().getRoot());
}
}

void ApplicationSolar::uploadView() {
	// bind shader to which to upload unforms
	glUseProgram(m_shaders.at("planet").handle);
  // vertices are transformed in camera space, so camera transform must be inverted
  glm::fmat4 view_matrix = glm::inverse(SceneGraph::getInstance().getActiveCamera()->getWorldTransform());
  // upload matrix to gpu
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));


  // bind shader to which to upload unforms
  glUseProgram(m_shaders.at("vao").handle);
  glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ViewMatrix"),
	  1, GL_FALSE, glm::value_ptr(view_matrix));

}

void ApplicationSolar::uploadProjection() {

	// bind shader to which to upload unforms
	glUseProgram(m_shaders.at("planet").handle);
  // upload matrix to gpu
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(SceneGraph::getInstance().getActiveCamera()->getProjectionMatrix()));
  
  // bind shader to which to upload unforms
  glUseProgram(m_shaders.at("vao").handle);
  glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ProjectionMatrix"),
	  1, GL_FALSE, glm::value_ptr(SceneGraph::getInstance().getActiveCamera()->getProjectionMatrix()));
}

// update uniform locations
void ApplicationSolar::uploadUniforms() { 

  // upload uniform values to new locations
  uploadView();
  uploadProjection();



}

///////////////////////////// intialisation functions /////////////////////////
// load shader sources
void ApplicationSolar::initializeShaderPrograms() {
  // store shader program objects in container
  m_shaders.emplace("planet", shader_program{{{GL_VERTEX_SHADER,m_resource_path + "shaders/simple.vert"},
                                           {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}}});
  // request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;

  //next Shader
  m_shaders.emplace("vao", shader_program{ {{GL_VERTEX_SHADER,m_resource_path + "shaders/vao.vert"},
										  {GL_FRAGMENT_SHADER, m_resource_path + "shaders/vao.frag"}} });
  // request uniform locations for shader program
  m_shaders.at("vao").u_locs["ModelMatrix"] = -1;
  m_shaders.at("vao").u_locs["ViewMatrix"] = -1;
  m_shaders.at("vao").u_locs["ProjectionMatrix"] = -1;
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
	GeometryNode* sun = new GeometryNode("Sun");

	sun->setLocalTransform(glm::fmat4{});
	sun->setModel(planet_model);
	sun->changeTimeDependency();
	//This should be optimized: once either a root is set or a child is added, the corresponding function should be set automatically
	root->addChildren(sun);
	sun->setParent(root);
	
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

	//moon
	//basically transformations work as with a planet, just that in this case the earth will be our center of rotation
	Node* moontranslation = new Node("Moon Translate");
	GeometryNode* moon= new GeometryNode("Moon Rotation");
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
	
	CameraNode* cam = new CameraNode("Camera");
	cam->setParent(root);
	root->addChildren(cam);
	cam->setEnabled(true);
	cam->setProjectionMatrix(m_view_projection);
	cam->setLocalTransform(m_view_transform);
	cam->setWorldTransform(m_view_transform);

	GeometryNode* stars = new GeometryNode("Stars");
	stars->setParent(root);
	stars->setStar(true);
	root->addChildren(stars);
	stars->setLocalTransform(glm::scale(glm::fmat4{}, glm::fvec3{ 20.0f, 20.0f, 20.0f }));
	std::cout<<SceneGraph::getInstance().printGraph();
}

// load models
void ApplicationSolar::initializeGeometry() {
  model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL);

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
  float  orbitresolution = 360.0f;
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
  orbit_object.num_elements = orbit_model.size()/6;
}

///////////////////////////// callback functions for window events ////////////
// handle key input
void ApplicationSolar::keyCallback(int key, int action, int mods) {
  if (key == GLFW_KEY_W  && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
	  SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::translate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), glm::fvec3{0.0f, 0.0f, -0.1f}));
    uploadView();
  }
  else if (key == GLFW_KEY_S  && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
	  SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::translate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), glm::fvec3{0.0f, 0.0f, 0.1f}));
    uploadView();
  }
  //add a few more controls to allow for rotations:
  else if (key == GLFW_KEY_X && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
	  SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::rotate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(),0.01f, glm::fvec3{ 1.0f, 0.0f, 0.0f }));
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
}

//handle delta mouse movement input
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  // mouse handling
	SceneGraph::getInstance().getActiveCamera()->setLocalTransform(glm::translate(SceneGraph::getInstance().getActiveCamera()->getLocalTransform(), glm::fvec3{ 0.001f*pos_x, 0.001f*pos_y, 0.0f }));
	uploadView();
}

//handle resizing
void ApplicationSolar::resizeCallback(unsigned width, unsigned height) {
  // recalculate projection matrix for new aspect ration
  SceneGraph::getInstance().getActiveCamera()->setProjectionMatrix(utils::calculate_projection_matrix(float(width) / float(height)));
  // upload new projection matrix
  uploadProjection();
}


// exe entry point
int main(int argc, char* argv[]) {
  Application::run<ApplicationSolar>(argc, argv, 3, 2);
}


