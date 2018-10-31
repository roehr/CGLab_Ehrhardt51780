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

std::string GeometryNode::getType() {
	return "Geometry";
}
