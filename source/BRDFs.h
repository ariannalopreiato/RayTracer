#pragma once
#include <cassert>
#include "Math.h"

namespace dae
{
	namespace BRDF
	{
		/**
		 * \param kd Diffuse Reflection Coefficient
		 * \param cd Diffuse Color
		 * \return Lambert Diffuse Color
		 */
		static ColorRGB Lambert(float kd, const ColorRGB& cd)
		{
			return (cd * kd) / float(M_PI);
		}

		static ColorRGB Lambert(const ColorRGB& kd, const ColorRGB& cd)
		{
			return (cd * kd) / float(M_PI);
		}

		/**
		 * \brief todo
		 * \param ks Specular Reflection Coefficient
		 * \param exp Phong Exponent
		 * \param l Incoming (incident) Light Direction
		 * \param v View Direction
		 * \param n Normal of the Surface
		 * \return Phong Specular Color
		 */
		static ColorRGB Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n)
		{		
			Vector3 reflect = Vector3::Reflect(n, l);
			float cosine = std::max(0.f, Vector3::Dot(reflect, v));
			float phong = ks * pow(cosine, exp);
			return ColorRGB{ phong, phong, phong };
		}

		/**
		 * \brief BRDF Fresnel Function >> Schlick
		 * \param h Normalized Halfvector between View and Light directions
		 * \param v Normalized View direction
		 * \param f0 Base reflectivity of a surface based on IOR (Indices Of Refrection), this is different for Dielectrics (Non-Metal) and Conductors (Metal)
		 * \return
		 */
		static ColorRGB FresnelFunction_Schlick(const Vector3& h, const Vector3& v, const ColorRGB& f0)
		{
			ColorRGB start = { 1 - f0.r, 1 - f0.g, 1 - f0.b };
			float dot = std::max(0.f, Vector3::Dot(h, v));
			auto power = powf(1 - dot, 5.f);
			return f0 + start * power;
		}

		/**
		 * \brief BRDF NormalDistribution >> Trowbridge-Reitz GGX (UE4 implemetation - squared(roughness))
		 * \param n Surface normal
		 * \param h Normalized half vector
		 * \param roughness Roughness of the material
		 * \return BRDF Normal Distribution Term using Trowbridge-Reitz GGX
		 */
		static float NormalDistribution_GGX(const Vector3& n, const Vector3& h, float roughness)
		{
			float dotSq = powf(std::max(0.f, Vector3::Dot(n, h)), 2.f);
			float denominator = dotSq * ((roughness * roughness) - 1) + 1;
			return (roughness * roughness) / (float(M_PI) * powf(denominator, 2.f));
		}

		/**
		 * \brief BRDF Geometry Function >> Schlick GGX (Direct Lighting + UE4 implementation - squared(roughness))
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using SchlickGGX
		 */
		static float GeometryFunction_SchlickGGX(const Vector3& n, const Vector3& v, float roughness)
		{
			float k = ((roughness + 1) * (roughness + 1)) / 8;
			float dotNormalView = std::max(0.f, Vector3::Dot(n, v));
			return dotNormalView / ((dotNormalView * (1 - k)) + k);
		}

		/**
		 * \brief BRDF Geometry Function >> Smith (Direct Lighting)
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param l Normalized light direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using Smith (> SchlickGGX(n,v,roughness) * SchlickGGX(n,l,roughness))
		 */
		static float GeometryFunction_Smith(const Vector3& n, const Vector3& v, const Vector3& l, float roughness)
		{
			float masking = GeometryFunction_SchlickGGX(n, v, roughness);
			float shadowing = GeometryFunction_SchlickGGX(n, l, roughness);
			return masking * shadowing;
		}

	}
}