#ifndef __BASIC_COLLIDER_H__
#define __BASIC_COLLIDER_H__

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

struct BoundingBox
{
	glm::vec3 center = glm::vec3(0.0f);
	float width, height, depth = 0.0f;

	/*
	public boolean boundingIntersection(Ray ray) {
            
        Point3D po = ray.get_POO();    
        double dirfra_x =  1.0f / ray.get_direction().get_head().get_x().get();
        double dirfra_y =  1.0f / ray.get_direction().get_head().get_y().get();
        double dirfra_z =  1.0f / ray.get_direction().get_head().get_z().get();


        // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
        // r.org is origin of ray
        double t1 = (min.get_x().get() - po.get_x().get()) * dirfra_x;
        double t2 = (max.get_x().get() - po.get_x().get()) * dirfra_x;
        double t3 = (min.get_y().get() - po.get_y().get()) * dirfra_y;
        double t4 = (max.get_y().get() - po.get_y().get()) * dirfra_y;
        double t5 = (min.get_z().get() - po.get_z().get()) * dirfra_z;
        double t6 = (max.get_z().get() - po.get_z().get()) * dirfra_z;


        double tmin = Math.max(Math.max(Math.min(t1, t2), Math.min(t3, t4)), Math.min(t5, t6));
        double tmax = Math.min(Math.min(Math.max(t1, t2), Math.max(t3, t4)), Math.max(t5, t6));

        // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
        if (tmax < 0)
        {
            return false;
        }
        // if tmin > tmax, ray doesn't intersect AABB
        if (tmin > tmax)
        {
            return false;
        }
        return true;
        
    }

	*/
	
	bool rayIntersects(const glm::vec3& origin, const glm::vec3& direction)
	{
        return false;
	}
};


#endif