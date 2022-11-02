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
			float a = Vector3::Dot(ray.direction, ray.direction);
			float b = Vector3::Dot(2 * ray.direction, ray.origin - sphere.origin);
			float c = Vector3::Dot(ray.origin - sphere.origin, ray.origin - sphere.origin) - (sphere.radius * sphere.radius);
			float discriminant = (b * b) - (4 * a * c);

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
			float hit = Vector3::Dot(plane.origin - ray.origin, plane.normal) / Vector3::Dot(ray.direction, plane.normal);
		
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
			Vector3 a = triangle.v1 - triangle.v0;
			Vector3 b = triangle.v2 - triangle.v1;
			Vector3 c = triangle.v0 - triangle.v2;

			//check if they are perpendicular
			if (Vector3::Dot(triangle.normal, ray.direction) == 0)
				return false;

			Vector3 center = (triangle.v0 + triangle.v1 + triangle.v2) / 3; //center of the triangle
			Vector3 L = center - ray.origin;

			float t = Vector3::Dot(L, triangle.normal) / Vector3::Dot(ray.direction, triangle.normal); //plane intersection
			if (t < ray.min || t > ray.max) //range check
				return false;

			//check if p is on the correct side of the triangle
			auto p = ray.origin + (t * ray.direction); //point inside of the triangle
			auto pointA = p - triangle.v0;
			auto pointB = p - triangle.v1;
			auto pointC = p - triangle.v2;

			if (Vector3::Dot(triangle.normal, Vector3::Cross(a, pointA)) < 0 || Vector3::Dot(triangle.normal, Vector3::Cross(b, pointB)) < 0
				|| Vector3::Dot(triangle.normal, Vector3::Cross(c, pointC)) < 0)
				return false;

			if (triangle.cullMode == TriangleCullMode::NoCulling && ignoreHitRecord)
				return true;

			if (ignoreHitRecord)
			{
				//back face
				if (Vector3::Dot(triangle.normal, ray.direction) > 0 && triangle.cullMode == TriangleCullMode::FrontFaceCulling)
					return false;

				//front face
				if (Vector3::Dot(triangle.normal, ray.direction) < 0 && triangle.cullMode == TriangleCullMode::BackFaceCulling)
					return false;
			}
			else
			{
				//back face
				if (Vector3::Dot(triangle.normal, ray.direction) > 0 && triangle.cullMode == TriangleCullMode::BackFaceCulling)
					return false;

				//front face
				if (Vector3::Dot(triangle.normal, ray.direction) < 0 && triangle.cullMode == TriangleCullMode::FrontFaceCulling)
					return false;
			}

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
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			bool result{};
			HitRecord currentHit{};
			int normalCount{};
			Triangle triangle{};
			triangle.cullMode = mesh.cullMode;
			triangle.materialIndex = mesh.materialIndex;

			for (int i = 0; i < mesh.indices.size(); ++i)
			{
				triangle.v0 = mesh.transformedPositions[mesh.indices[i]];
				triangle.v1 = mesh.transformedPositions[mesh.indices[++i]];
				triangle.v2 = mesh.transformedPositions[mesh.indices[++i]];

				triangle.normal = mesh.transformedNormals[normalCount];

				bool current = HitTest_Triangle(triangle, ray, hitRecord, ignoreHitRecord); //check if triangle hits

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
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
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
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.emplace_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}