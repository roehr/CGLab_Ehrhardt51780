#ifndef GEOMETRY_NODE_HPP
#define GEOMETRY_NODE_HPP
#include "model.hpp"
#include "Node.hpp"
class GeometryNode : public Node
{
private:
	model geometry;
public:
	GeometryNode();
	GeometryNode(std::string);
	~GeometryNode();
	model getModel();
	void setModel(model);
	std::string getType();
};

#endif