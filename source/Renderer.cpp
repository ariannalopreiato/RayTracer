//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();
	
	float fov = std::tan((camera.fovAngle * M_PI / 180) / 2);
	float aspectRatio = float(m_Width) / float(m_Height);

	for (int px{}; px < m_Width; ++px)
	{
		float x = float(((2 * (px + 0.5)) / m_Width) - 1) * aspectRatio * fov;

		for (int py{}; py < m_Height; ++py)
		{
			float y = (1 - float((2 * (py + 0.5)) / m_Height)) * fov;

			//float gradient = px / static_cast<float>(m_Width);
			//gradient += py / static_cast<float>(m_Width);
			//gradient /= 2.0f;
			//ColorRGB finalColor{ gradient, gradient, gradient };
			//Vector3 rayDirection{ x, y, 1 };
			//Ray hitRay{ {0,0,0}, rayDirection };
			//ColorRGB finalColor{ rayDirection.x, rayDirection.y, rayDirection.z };

			const Matrix cameraToWorld = camera.CalculateCameraToWorld();
			Vector3 rayDirection = cameraToWorld.TransformVector({ x,y,1 }).Normalized();
			Ray viewRay{ camera.origin, rayDirection };
			ColorRGB finalColor{};
			HitRecord closestHit{};
			pScene->GetClosestHit(viewRay, closestHit);

			if (closestHit.didHit)
			{
				auto lights = pScene->GetLights(); //get all the lights of the scene

				for (const auto& light : lights) //loop all lights
				{
					Vector3 startPoint = closestHit.origin + closestHit.normal * 0.01f; //the point that just got hit
					Vector3 direction = LightUtils::GetDirectionToLight(light, startPoint); //vector from hit point to light
					Ray lightRay{ startPoint, direction }; //calculate the light ray
					lightRay.max = lightRay.direction.Normalize();
					float lambertLaw = Vector3::Dot(closestHit.normal, direction.Normalized());

					if (pScene->DoesHit(lightRay) && m_ShadowsEnabled)
						continue;

					ColorRGB radiance = LightUtils::GetRadiance(light, startPoint);
					ColorRGB brdf = materials[closestHit.materialIndex]->Shade(closestHit, lightRay.direction, -rayDirection);

					switch (m_CurrentLightingMode)
					{
					case LightingMode::ObservedArea:
						if (lambertLaw > 0)
							finalColor += {lambertLaw, lambertLaw, lambertLaw};
						break;
					case LightingMode::Radiance:
						finalColor += radiance;
						break;
					case LightingMode::BRDF:
						finalColor += brdf;
						break;
					case LightingMode::Combined:
						if (lambertLaw > 0)
							finalColor += radiance * brdf * lambertLaw;
						break;
					}

				}
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void dae::Renderer::CycleLightMode()
{
	switch (m_CurrentLightingMode)
	{
	case LightingMode::Combined:
		m_CurrentLightingMode = LightingMode::ObservedArea;
		break;
	case LightingMode::ObservedArea:
		m_CurrentLightingMode = LightingMode::Radiance;
		break;
	case LightingMode::Radiance:
		m_CurrentLightingMode = LightingMode::BRDF;
		break;
	case LightingMode::BRDF:
		m_CurrentLightingMode = LightingMode::Combined;
		break;
	}
}
