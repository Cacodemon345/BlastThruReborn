// SAT collision check and resolution
// MIT license
// Usage example:
// if (testCollision(obb1, obb2, mtv)) { // obb1, obb2 - sf::RectangleShape, mtv - sf::Vector2f
//      obb1.move(mtv);
// }

// Originally by eliasdaler.

#include <array>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cassert>

static const float NORMAL_TOLERANCE = 0.0001f;
using RectVertexArray = std::array<sf::Vector2f, 4>;

inline float getLength(const sf::Vector2f& v)
{
	return std::sqrt(v.x * v.x + v.y * v.y);
}

// Returns normalized vector
inline sf::Vector2f getNormalized(const sf::Vector2f& v)
{
	float length = getLength(v);
	if (length < NORMAL_TOLERANCE) { return sf::Vector2f(); }
	return sf::Vector2f(v.x / length, v.y / length);
}

inline float dotProduct(const sf::Vector2f& a, const sf::Vector2f& b)
{
	return a.x * b.x + a.y * b.y;
}

// Returns right hand perpendicular vector
inline sf::Vector2f getNormal(const sf::Vector2f& v)
{
	return sf::Vector2f(-v.y, v.x);
}

// Find minimum and maximum projections of each vertex on the axis
inline sf::Vector2f projectOnAxis(const RectVertexArray& vertices, const sf::Vector2f& axis)
{
	float min = std::numeric_limits<float>::infinity();
	float max = -std::numeric_limits<float>::infinity();
	for (auto& vertex : vertices) {
		float projection = dotProduct(vertex, axis);
		if (projection < min) { min = projection; }
		if (projection > max) { max = projection; }
	}
	return sf::Vector2f(min, max);
}

// a and b are ranges and it's assumed that a.x <= a.y and b.x <= b.y
inline bool areOverlapping(const sf::Vector2f& a, const sf::Vector2f& b)
{
	return a.x <= b.y && a.y >= b.x;
}

// a and b are ranges and it's assumed that a.x <= a.y and b.x <= b.y
inline float getOverlapLength(const sf::Vector2f& a, const sf::Vector2f& b)
{
	if (!areOverlapping(a, b)) { return 0.f; }
	return std::min(a.y, b.y) - std::max(a.x, b.x);
}

inline sf::Vector2f getCenter(const sf::RectangleShape& shape)
{
	const sf::Transform& transform = shape.getTransform();
	sf::FloatRect local = shape.getLocalBounds();
	return transform.transformPoint(local.width / 2.f, local.height / 2.f);
}

inline RectVertexArray getVertices(const sf::RectangleShape& shape)
{
	RectVertexArray vertices;
	const sf::Transform& transform = shape.getTransform();
	for (std::size_t i = 0u; i < shape.getPointCount(); ++i) {
		vertices[i] = transform.transformPoint(shape.getPoint(i));
	}
	return vertices;
}

inline sf::Vector2f getPerpendicularAxis(const RectVertexArray& vertices, std::size_t index)
{
	assert(index >= 0 && index < 4); // rect has 4 possible axes
	return getNormal(getNormalized(vertices[index + 1] - vertices[index]));
}

// axes for which we'll test stuff. Two for each box, because testing for parallel axes isn't needed
inline std::array<sf::Vector2f, 4> getPerpendicularAxes(const RectVertexArray& vertices1, const RectVertexArray& vertices2)
{
	std::array<sf::Vector2f, 4> axes;

	axes[0] = getPerpendicularAxis(vertices1, 0);
	axes[1] = getPerpendicularAxis(vertices1, 1);

	axes[2] = getPerpendicularAxis(vertices2, 0);
	axes[3] = getPerpendicularAxis(vertices2, 1);
	return axes;
}

// Separating Axis Theorem (SAT) collision test
// Minimum Translation Vector (MTV) is returned for the first Oriented Bounding Box (OBB)
inline bool testCollision(const sf::RectangleShape& obb1, const sf::RectangleShape& obb2, sf::Vector2f& mtv)
{
	RectVertexArray vertices1 = getVertices(obb1);
	RectVertexArray vertices2 = getVertices(obb2);
	std::array<sf::Vector2f, 4> axes = getPerpendicularAxes(vertices1, vertices2);

	// we need to find the minimal overlap and axis on which it happens
	float minOverlap = std::numeric_limits<float>::infinity();
	
	for (auto& axis : axes) {
		sf::Vector2f proj1 = projectOnAxis(vertices1, axis);
		sf::Vector2f proj2 = projectOnAxis(vertices2, axis);

		float overlap = getOverlapLength(proj1, proj2);
		if (overlap == 0.f) { // shapes are not overlapping
			mtv = sf::Vector2f();
			return false;
		} else {
			if (overlap < minOverlap) {
				minOverlap = overlap;
				mtv = axis * minOverlap; 
				// ideally we would do this only once for the minimal overlap
				// but this is very cheap operation
			}
		}
	}

	// need to reverse MTV if center offset and overlap are not pointing in the same direction
	bool notPointingInTheSameDirection = dotProduct(getCenter(obb1) - getCenter(obb2), mtv) < 0;
	if (notPointingInTheSameDirection) { mtv = -mtv; }
	return true;
}