//
//  AABox.h - Axis Aligned Boxes
//  hifi
//
//  Added by Brad Hefta-Gaub on 04/11/13.
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//
//  Simple axis aligned box class.
//

#ifndef _AABOX_
#define _AABOX_

#include <glm/glm.hpp>

class AABox 
{

public:

	AABox(const glm::vec3& corner, float x, float y, float z) : _corner(corner), _size(x,y,z) { };
	AABox(const glm::vec3& corner, const glm::vec3& size) : _corner(corner), _size(size) { };
    AABox() : _corner(0,0,0), _size(0,0,0) { }
    ~AABox() { }

	void setBox(const glm::vec3& corner, float x, float y, float z) { setBox(corner,glm::vec3(x,y,z)); };
	void setBox(const glm::vec3& corner, const glm::vec3& size);

	// for use in frustum computations
	glm::vec3 getVertexP(const glm::vec3& normal) const;
	glm::vec3 getVertexN(const glm::vec3& normal) const;
	
	void scale(float scale);
	
	const glm::vec3& getCorner() const { return _corner; };
	const glm::vec3& getSize() const { return _size; };
	const glm::vec3& getCenter() const { return _center; };

private:
	glm::vec3 _corner;
	glm::vec3 _center;
	glm::vec3 _size;
};


#endif