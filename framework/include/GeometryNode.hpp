#ifndef GEOMETRY_NODE_HPP
#define GEOMETRY_NODE_HPP
#include "model.hpp"
#include "structs.hpp"
#include "Node.hpp"
class GeometryNode : public Node
{
private:
	glm::fvec3 color;
	model geometry;
	bool isStar = false;
	bool isOrbit = false;
	bool isSkybox = false;
	bool isSun = false;
	GLuint diffTex = 0;
	GLuint normTex = 0;
	GLuint specTex = 0;
	GLuint dispTex = 0;
public:

	//now this is badly hacked and needs a cleanup. Instead of defining checkups which type of object is used, the classes should be adjusted
	//Either it is possible to keep the modelsolution as is or create a new Node Type (ArrayNode or something) - unfortunatly not happening yet due to lack of time
	//Best solution most likely:
	//New nodetypes -> Geometry as Base - ModelNode for Stars/Orbit (ArrayNode)
	//shaders and model_object (and coming stuff as Textures) as part of the GeometryNode
	//Model for ModelNode
	//Container for ArrayNode
	//That way other types of Objects should be addable easily and the Application can be cleaned up a bit
	bool useStar();
	bool useOrbit();
	bool useSkybox();
	bool useSun();
	glm::fvec3 getColor();
	void setColor(glm::fvec3);
	void setStar(bool);
	void setOrbit(bool);
	void setSun(bool);
	void setSkybox(bool);
	GeometryNode();
	GeometryNode(std::string);
	~GeometryNode();
	
	model getModel();
	void setModel(model);
	model getModelObject();
	void setModel(model_object);
	void setDiffTex(GLuint);
	GLuint getDiffTex();
	GLuint* getDiffTexptr();

	void setNormTex(GLuint);
	GLuint getNormTex();
	GLuint* getNormTexptr();

	void setSpecTex(GLuint);
	GLuint getSpecTex();
	GLuint* getSpecTexptr();

	void setDispTex(GLuint);
	GLuint getDispTex();
	GLuint* getDispTexptr();

	std::string getType();

};

#endif