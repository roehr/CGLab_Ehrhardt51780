#ifndef APPLICATION_SOLAR_HPP
#define APPLICATION_SOLAR_HPP

#include "application.hpp"
#include "model.hpp"
#include "structs.hpp"
#include "Node.hpp"
#include "GeometryNode.hpp"

// gpu representation of model
class ApplicationSolar : public Application {
 public:
  // allocate and initialize objects
  ApplicationSolar(std::string const& resource_path);
  // free allocated objects
  ~ApplicationSolar();

  // react to key input
  void keyCallback(int key, int action, int mods);
  //handle delta mouse movement input
  void mouseCallback(double pos_x, double pos_y);
  //handle resizing
  void resizeCallback(unsigned width, unsigned height);

  // draw all objects
  void render() const;
  void renderSceneGraph(Node*) const;

 protected:
  void initializeShaderPrograms();
  void initializeGeometry();
  void initializeSceneGraph();
  void initializefbo();
  void initializePlanetTextures(GeometryNode*, std::string, std::string, std::string, std::string);
  void initializeSkyboxTextures(GeometryNode*);
  // update uniform values
  void uploadUniforms();
  // upload projection matrix
  void uploadProjection();
  // upload view matrix
  void uploadView();
  
  // cpu representation of model
  model_object planet_object;
  model_object star_object;
  model_object orbit_object;
  model_object skybox_object;
  model_object screenquad_object;
  bool mirrorx = false;
  bool mirrory = false;
  bool gauss = false;
  bool grey = false;
  bool offScreen = true;
  bool fullScreen = false;
  unsigned int framebuffer;
  unsigned int colorbuffer;
  unsigned int renderbuffer;
  bool isStar;
  bool isOrbit;
  // camera transform matrix
  glm::fmat4 m_view_transform;
  // camera projection matrix
  glm::fmat4 m_view_projection;
  float  orbitresolution = 360.0f; //defines how many segments the circle which represents the orbits will get
};

#endif