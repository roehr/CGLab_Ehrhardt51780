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
