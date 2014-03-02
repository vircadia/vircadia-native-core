//
//  AABoundingVolume.h - Axis Aligned Bounding Volumes
//  hifi
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef _AABOUNDING_VOLUME_
#define _AABOUNDING_VOLUME_

#include <glm/glm.hpp>



// A simple axis aligned bounding volume.
#define AABF_NUM_DIMS 3
#define AABF_LOW 0
#define AABF_HIGH 1
//Todo:: make this a template.
class AABoundingVolume
{
public:
	AABoundingVolume() : _numPointsInSet(0), _isSingleDirectionSet(false) { memset(_bounds,0,sizeof(_bounds)); }
	~AABoundingVolume(){}

	void AddToSet(const glm::vec3 newPt) 
	{ 
		if (_numPointsInSet == 0)
		{
			for (int i = 0; i < AABF_NUM_DIMS; i++)
			{
				_bounds[i][AABF_LOW] = _bounds[i][AABF_HIGH] = newPt[i];
			}
		}
		else
		{
			for (int i = 0; i < AABF_NUM_DIMS; i++)
			{
				if (newPt[i] < _bounds[i][AABF_LOW]) _bounds[i][AABF_LOW] = newPt[i];
				if (newPt[i] > _bounds[i][AABF_HIGH]) _bounds[i][AABF_HIGH] = newPt[i];
			}
		}
		_numPointsInSet++;
	}

	float getBound(const int dim, const int lohi) { return _bounds[dim][lohi]; }
	glm::vec3 getCorner(const BoxVertex i)
	{
		switch (i)
		{
		case BOTTOM_LEFT_NEAR:
			return glm::vec3(_bounds[0][AABF_LOW], _bounds[1][AABF_LOW], _bounds[2][AABF_HIGH]);
			break;
		case BOTTOM_RIGHT_NEAR:
			return glm::vec3(_bounds[0][AABF_HIGH], _bounds[1][AABF_LOW], _bounds[2][AABF_HIGH]);
			break;
		case TOP_RIGHT_NEAR:
			return glm::vec3(_bounds[0][AABF_HIGH], _bounds[1][AABF_HIGH], _bounds[2][AABF_HIGH]);
			break;
		case TOP_LEFT_NEAR:
			return glm::vec3(_bounds[0][AABF_LOW], _bounds[1][AABF_HIGH], _bounds[2][AABF_HIGH]);
			break;
		case BOTTOM_LEFT_FAR:
			return glm::vec3(_bounds[0][AABF_LOW], _bounds[1][AABF_LOW], _bounds[2][AABF_LOW]);
			break;
		case BOTTOM_RIGHT_FAR:
			return glm::vec3(_bounds[0][AABF_HIGH], _bounds[1][AABF_LOW], _bounds[2][AABF_LOW]);
			break;
		case TOP_RIGHT_FAR:
			return glm::vec3(_bounds[0][AABF_HIGH], _bounds[1][AABF_HIGH], _bounds[2][AABF_LOW]);
			break;
		case TOP_LEFT_FAR:
			return glm::vec3(_bounds[0][AABF_LOW], _bounds[1][AABF_HIGH], _bounds[2][AABF_LOW]);
			break;
		default : 
			assert(0 && "Requested invalid bounding volume vertex!");
		}
		return glm::vec3(-1.0, -1.0, -1.0);
	}

	void setIsSingleDirection(const bool enable, const glm::vec3 normal)
	{
		if (enable) _singleDirectionNormal = normal;
		_isSingleDirectionSet = true;
	}

	int within(const float loc, int dim)
	{
		if (loc < _bounds[dim][AABF_LOW]) return -1;
		if (loc > _bounds[dim][AABF_HIGH]) return 1;
		return 0;
	}

	bool isSingleDirectionSet(){ return _isSingleDirectionSet; }
	glm::vec3 getSingleDirectionNormal(){ return _singleDirectionNormal; }

	AABoundingVolume& operator= (const AABoundingVolume& val) {

		if (this !=&val)

		{			

			memcpy(_bounds, &val._bounds, sizeof(AABoundingVolume));

		}

		return *this;
	}
	
protected:
	float	_bounds[AABF_NUM_DIMS][2]; // Bounds for each dimension. NOTE:: Not a vector so don't store it that way!
	int		_numPointsInSet;
	bool	_isSingleDirectionSet;
	glm::vec3	_singleDirectionNormal;
};

#define AABF2D_NUM_DIMS 2
#define UNDEFINED_RANGE_VAL 99999.0
class AA2DBoundingVolume
{
public:
	AA2DBoundingVolume() : _numPointsInSet(0) {}
	~AA2DBoundingVolume(){}

	void AddToSet(const float newPt[2]) 
	{ 
		if (_numPointsInSet == 0)
		{
			for (int i = 0; i < AABF2D_NUM_DIMS; i++)
			{
				_bounds[i][AABF_LOW] = _bounds[i][AABF_HIGH] = newPt[i];
			}
		}
		else
		{
			for (int i = 0; i < AABF2D_NUM_DIMS; i++)
			{
				if (newPt[i] < _bounds[i][AABF_LOW]) _bounds[i][AABF_LOW] = newPt[i];
				if (newPt[i] > _bounds[i][AABF_HIGH]) _bounds[i][AABF_HIGH] = newPt[i];
			}
		}
		_numPointsInSet++;
	}

	// return true if its in range.
	bool clipToRegion(const float lowx, const float lowy, const float highx, const float highy)
	{
		assert(highx > lowx && highy > lowy);
		bool inRange = true;
		if (_bounds[0][AABF_LOW] > highx || _bounds[0][AABF_HIGH] < lowx)
		{ 
			_bounds[0][AABF_LOW] = _bounds[0][AABF_HIGH] = UNDEFINED_RANGE_VAL; 
			inRange=false;
		}
		else
		{
			if (_bounds[0][AABF_LOW] < lowx) _bounds[0][AABF_LOW] = lowx;
			if (_bounds[0][AABF_HIGH] > highx) _bounds[0][AABF_HIGH] = highx;
		}
		if (_bounds[1][AABF_LOW] > highy || _bounds[1][AABF_HIGH] < lowy)
		{ 
			_bounds[1][AABF_LOW] = _bounds[1][AABF_HIGH] = UNDEFINED_RANGE_VAL; 
			inRange=false;
		}
		else
		{		
			if (_bounds[1][AABF_LOW] < lowy) _bounds[1][AABF_LOW] = lowy;
			if (_bounds[1][AABF_HIGH] > highy) _bounds[1][AABF_HIGH] = highy;
		}
		return inRange;
	}

	float getHeight()
	{ 
		if (_bounds[1][AABF_HIGH] == UNDEFINED_RANGE_VAL || _bounds[1][AABF_LOW] == UNDEFINED_RANGE_VAL) return 0; 
		return _bounds[1][AABF_HIGH] - _bounds[1][AABF_LOW]; 
	}
	float getWidth()
	{ 
		if (_bounds[0][AABF_HIGH] == UNDEFINED_RANGE_VAL || _bounds[0][AABF_LOW] == UNDEFINED_RANGE_VAL) return 0; 
		return _bounds[0][AABF_HIGH] - _bounds[0][AABF_LOW]; 
	}

protected:
	float	_bounds[AABF2D_NUM_DIMS][2];
	int		_numPointsInSet;
};

#endif
