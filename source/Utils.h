#pragma once
#include <cassert>
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//intersection of ray with sphere
			const float a = Vector3::Dot(ray.direction, ray.direction);
			const float b = Vector3::Dot(2 * ray.direction, ray.origin - sphere.origin);
			const float c = Vector3::Dot(ray.origin - sphere.origin, ray.origin - sphere.origin) - (sphere.radius * sphere.radius);
			const float discriminant = (b * b) - (4 * a * c);

			if (discriminant <= 0.f) //the ray does not intersect the sphere or is tangent to it
				return false;

			float hit = (-b - sqrt(discriminant)) / (2 * a);
			if (hit < ray.min)
				hit = (-b + sqrt(discriminant)) / (2 * a);
			if (hit >= ray.min && hit < ray.max)
			{
				if (ignoreHitRecord) //if hitRecord is ignored 
					return true;

				hitRecord.t = hit;
				hitRecord.origin = ray.origin + (hit * ray.direction);
				hitRecord.didHit = true;
				hitRecord.materialIndex = sphere.materialIndex; //gives to the pixel the material of the object it hits
				hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
				return true;
			}
			return false;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//intersection of ray with plane
			const float hit = Vector3::Dot(plane.origin - ray.origin, plane.normal) / Vector3::Dot(ray.direction, plane.normal);
		
			if (hit >= ray.min && hit < ray.max)
			{
				if (ignoreHitRecord) //if hitRecord is ignored 
					return true;

				hitRecord.t = hit;
				hitRecord.origin = ray.origin + (hit * ray.direction);
				hitRecord.didHit = true;
				hitRecord.materialIndex = plane.materialIndex; //gives to the pixel the material of the object it hits
				hitRecord.normal = plane.normal;
				return true;
			}
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3 a{ triangle.v1 - triangle.v0 };
			const Vector3 b{ triangle.v2 - triangle.v1 };
			const Vector3 c{ triangle.v0 - triangle.v2 };

			//check if they are perpendicular
			if (Vector3::Dot(triangle.normal, ray.direction) == 0)
				return false;

			const Vector3 center{ (triangle.v0 + triangle.v1 + triangle.v2) / 3 }; //center of the triangle
			const Vector3 L{ center - ray.origin };

			const float t{ Vector3::Dot(L, triangle.normal) / Vector3::Dot(ray.direction, triangle.normal) }; //plane intersection
			if (t < ray.min || t > ray.max) //range check
				return false;

			//check if p is on the correct side of the triangle
			const Vector3 p{ ray.origin + (t * ray.direction) }; //point inside of the triangle
			const Vector3 pointA{ p - triangle.v0 };
			const Vector3 pointB{ p - triangle.v1 };
			const Vector3 pointC{ p - triangle.v2 };

			if (Vector3::Dot(triangle.normal, Vector3::Cross(a, pointA)) < 0 || Vector3::Dot(triangle.normal, Vector3::Cross(b, pointB)) < 0
				|| Vector3::Dot(triangle.normal, Vector3::Cross(c, pointC)) < 0)
				return false;


			if (triangle.cullMode == TriangleCullMode::NoCulling && ignoreHitRecord)
				return true;

			if (Vector3::Dot(triangle.normal, ray.direction) > 0)
			{
				if ((triangle.cullMode == TriangleCullMode::FrontFaceCulling && ignoreHitRecord) || triangle.cullMode == TriangleCullMode::BackFaceCulling)
					return false;
			}

			if (Vector3::Dot(triangle.normal, ray.direction) < 0)
			{
				if ((triangle.cullMode == TriangleCullMode::FrontFaceCulling) || triangle.cullMode == TriangleCullMode::BackFaceCulling && ignoreHitRecord)
					return false;
			}

			//if (ignoreHitRecord)
			//{
			//	if (triangle.cullMode == TriangleCullMode::NoCulling)
			//		return true;
			//	//back face
			//	if (Vector3::Dot(triangle.normal, ray.direction) > 0 && triangle.cullMode == TriangleCullMode::FrontFaceCulling)
			//		return false;
			//	//front face
			//	if (Vector3::Dot(triangle.normal, ray.direction) < 0 && triangle.cullMode == TriangleCullMode::BackFaceCulling)
			//		return false;
			//}
			//else
			//{
			//	//back face
			//	if (Vector3::Dot(triangle.normal, ray.direction) > 0 && triangle.cullMode == TriangleCullMode::BackFaceCulling)
			//		return false;
			//	//front face
			//	if (Vector3::Dot(triangle.normal, ray.direction) < 0 && triangle.cullMode == TriangleCullMode::FrontFaceCulling)
			//		return false;
			//}

			hitRecord.didHit = true;
			hitRecord.materialIndex = triangle.materialIndex;
			hitRecord.normal = triangle.normal;
			hitRecord.t = t; 
			hitRecord.origin = p;

			return true;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest

		inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			const float tx1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
			const float tx2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;

			float tmin = std::min(tx1, tx2);
			float tmax = std::max(tx1, tx2);

			const float ty1 = (mesh.transformedMinAABB.y - ray.origin.y) / ray.direction.y;
			const float ty2 = (mesh.transformedMaxAABB.y - ray.origin.y) / ray.direction.y;

			tmin = std::max(tmin, std::min(ty1, ty2));
			tmax = std::min(tmax, std::max(ty1, ty2));

			const float tz1 = (mesh.transformedMinAABB.z - ray.origin.z) / ray.direction.z;
			const float tz2 = (mesh.transformedMaxAABB.z - ray.origin.z) / ray.direction.z;

			tmin = std::max(tmin, std::min(tz1, tz2));
			tmax = std::min(tmax, std::max(tz1, tz2));

			return tmax > 0 && tmax >= tmin;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//slabtest
			if (!SlabTest_TriangleMesh(mesh, ray))
				return false;

			HitRecord currentHit{};
			int normalCount{};
			Triangle triangle{};
			triangle.cullMode = mesh.cullMode;
			triangle.materialIndex = mesh.materialIndex;

			for (size_t i = 0; i < mesh.indices.size(); ++i)
			{
				triangle.v0 = mesh.transformedPositions[mesh.indices[i]];
				triangle.v1 = mesh.transformedPositions[mesh.indices[++i]];
				triangle.v2 = mesh.transformedPositions[mesh.indices[++i]];

				triangle.normal = mesh.transformedNormals[normalCount];

				const bool current{ HitTest_Triangle(triangle, ray, hitRecord, ignoreHitRecord) }; //check if triangle hits

				++normalCount;

				if (current) //if it hits return true
					return current;
			}
			return hitRecord.didHit;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3& origin)
		{
			if (light.type == LightType::Directional) //directional light doesn't have an origin
				return -light.direction.Normalized() * FLT_MAX;
			else
				return light.origin - origin;
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			if (light.type == LightType::Directional)
				return light.color * light.intensity;
			else
			{
				float distanceSquared = (light.origin - target).SqrMagnitude();
				return light.color * (light.intensity / distanceSquared);
			}
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.emplace_back((int)i0 - 1);
					indices.emplace_back((int)i1 - 1);
					indices.emplace_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof()) 
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if(isnan(normal.x))
				{
					//int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					//int k = 0;
				}

				normals.emplace_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}