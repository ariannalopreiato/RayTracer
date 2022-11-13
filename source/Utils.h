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
			const Vector3 l = sphere.origin - ray.origin;
			const float tca = Vector3::Dot(l, ray.direction);
			if (tca < 0)
				return false;

			const float od = Vector3::Dot(Vector3::Reject(l, ray.direction), Vector3::Reject(l, ray.direction));
			if (od > sphere.radius * sphere.radius)
				return false;

			const float thc = sqrt((sphere.radius * sphere.radius) - od);
			float t = tca - thc;

			if (t < ray.min)
				t = tca + thc;

			if (t >= ray.min && t <= ray.max)
			{
				if (ignoreHitRecord) //if hitRecord is ignored 
					return true;

				hitRecord.t = t;
				hitRecord.origin = ray.origin + (t * ray.direction);
				hitRecord.didHit = true;
				hitRecord.materialIndex = sphere.materialIndex; //gives to the pixel the material of the object it hits
				hitRecord.normal = (hitRecord.origin - sphere.origin) / sphere.radius;
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
		//https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float dotNV{ Vector3::Dot(triangle.normal, ray.direction) };

			if (dotNV == 0)
				return false;

			if (ignoreHitRecord)
			{
				if (dotNV > 0 && triangle.cullMode == TriangleCullMode::FrontFaceCulling)
					return false;

				if (dotNV < 0 && triangle.cullMode == TriangleCullMode::BackFaceCulling)
					return false;
			}
			else
			{
				if (dotNV < 0 && triangle.cullMode == TriangleCullMode::FrontFaceCulling)
					return false;

				if (dotNV > 0 && triangle.cullMode == TriangleCullMode::BackFaceCulling)
					return false;
			}

			const Vector3 firstEdge{ triangle.v1 - triangle.v0 };
			const Vector3 secondEdge{ triangle.v2 - triangle.v0 };

			const Vector3 h{ Vector3::Cross(ray.direction, secondEdge) };
			const float a{ Vector3::Dot(h, firstEdge) };

			//check if the ray is parallel to the triangle
			if (a > -0.01f && a < 0.01f)
				return false;

			const float f{ 1.0f / a };
			const Vector3 s{ ray.origin - triangle.v0 };
			const float u{ f * Vector3::Dot(s, h) };

			if (u < 0.f || u > 1.f)
				return false;

			const Vector3 q{ Vector3::Cross(s, firstEdge) };
			const float v{ f * Vector3::Dot(ray.direction, q) };

			if (v < 0.f || u + v > 1.f)
				return false;

			float t{ f * Vector3::Dot(secondEdge, q) };

			if (t > hitRecord.t)
				return false;

			if (t < ray.min || t > ray.max)
				return false;

			if (ignoreHitRecord)
				return true;

			hitRecord.t = t;
			hitRecord.didHit = true;
			hitRecord.materialIndex = triangle.materialIndex;
			hitRecord.normal = triangle.normal;
			hitRecord.origin = ray.origin + ray.direction * t;

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
			float tx1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
			float tx2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;

			float tMin = std::min(tx1, tx2);
			float tMax = std::max(tx1, tx2);

			float ty1 = (mesh.transformedMinAABB.y - ray.origin.y) / ray.direction.y;
			float ty2 = (mesh.transformedMaxAABB.y - ray.origin.y) / ray.direction.y;

			tMin = std::max(tMin, std::min(ty1, ty2));
			tMax = std::min(tMax, std::max(ty1, ty2));

			float tz1 = (mesh.transformedMinAABB.z - ray.origin.z) / ray.direction.z;
			float tz2 = (mesh.transformedMaxAABB.z - ray.origin.z) / ray.direction.z;

			tMin = std::max(tMin, std::min(tz1, tz2));
			tMax = std::min(tMax, std::max(tz1, tz2));

			return tMax > 0 && tMax >= tMin;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//slabtest
			if (!SlabTest_TriangleMesh(mesh, ray))
				return false;

			bool result{ false };
			Triangle triangle{};
			triangle.cullMode = mesh.cullMode;
			triangle.materialIndex = mesh.materialIndex;

			for (size_t i = 0; i < mesh.indices.size(); ++i)
			{
				triangle.normal = mesh.transformedNormals[i/3];
				triangle.v0 = mesh.transformedPositions[mesh.indices[i]];
				triangle.v1 = mesh.transformedPositions[mesh.indices[++i]];
				triangle.v2 = mesh.transformedPositions[mesh.indices[++i]];			

				if (HitTest_Triangle(triangle, ray, hitRecord, ignoreHitRecord)) //check if triangle hits
					result = true;
			}
	
			return result;
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
				//if (sCommand == "#")
				//{
				//	// Ignore Comment
				//}
				//else 
				if (sCommand == "v")
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

				normal.Normalize();
				normals.emplace_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}