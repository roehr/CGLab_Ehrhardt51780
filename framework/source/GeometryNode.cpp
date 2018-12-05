#include "GeometryNode.hpp"



GeometryNode::GeometryNode()
{
}
GeometryNode::GeometryNode(std::string _name)
{
	Node::setName(_name);
}


GeometryNode::~GeometryNode()
{
}

model GeometryNode::getModel()
{
	return geometry;
}

void GeometryNode::setModel(model _geo)
{
	geometry = _geo;
}
void GeometryNode::setStar(bool val) {
	isStar = val;
}
void GeometryNode::setOrbit(bool val) {
	isOrbit = val;
}
bool GeometryNode::useStar() {
	return isStar;
}
bool GeometryNode::useOrbit() {
	return isOrbit;
}
glm::fvec3 GeometryNode::getColor()
{
	return color;
}
void GeometryNode::setColor(glm::fvec3 col)
{
	color = col;
}
std::string GeometryNode::getType() {
	return "Geometry";
}

GLuint GeometryNode::getDiffTex() {

	return diffTex;

}

GLuint* GeometryNode::getDiffTexptr() {

	return &diffTex;

}


 void GeometryNode::setDiffTex(GLuint diff) {

	 diffTex = diff;

}



 GLuint GeometryNode::getNormTex() {

	 return normTex;

 }

 GLuint* GeometryNode::getNormTexptr() {

	 return &normTex;

 }


 void GeometryNode::setNormTex(GLuint norm) {

	 normTex = norm;

 }


 GLuint GeometryNode::getSpecTex() {

	 return specTex;

 }

 GLuint* GeometryNode::getSpecTexptr() {

	 return &specTex;

 }


 void GeometryNode::setSpecTex(GLuint spec) {

	 specTex = spec;

 }


 GLuint GeometryNode::getDispTex() {

	 return dispTex;

 }

 GLuint* GeometryNode::getDispTexptr() {

	 return &dispTex;

 }


 void GeometryNode::setDispTex(GLuint disp) {

	 dispTex = disp;

 }


bool GeometryNode::useSkybox() {

	return isSkybox;

 }

void GeometryNode::setSkybox(bool val) {

	isSkybox = val;

}

bool GeometryNode::useSun() {

	return isSun;

}

void GeometryNode::setSun(bool val) {

	isSun = val;

}









