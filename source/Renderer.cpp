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

#include <future> //async
#include <ppl.h> //parallel for

using namespace dae;

#define ASYNC
//#define PARALLEL_FOR

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
	Camera& camera{ pScene->GetCamera() };
	camera.CalculateCameraToWorld();

	const float fovAngle{ camera.fovAngle * TO_RADIANS };
	const float fov{ tan(fovAngle / 2.f) };
	const float aspectRatio{ float(m_Width) / float(m_Height) };

	const auto& materials{ pScene->GetMaterials() };
	const auto& lights{ pScene->GetLights() };

	const uint32_t numPixels{ uint32_t(m_Width * m_Height) };

#if defined(ASYNC)
	//async exeution
	const uint32_t numCores = std::thread::hardware_concurrency();
	std::vector<std::future<void>> async_futures{};

	const uint32_t numPixelPerTask = numPixels / numCores;
	uint32_t numUnassignedPixels = numPixels % numCores;
	uint32_t currPixelIdx{ 0 };

	for (uint32_t coreId{ 0 }; coreId < numCores; ++coreId)
	{
		uint32_t taskSize = numPixelPerTask;
		if (numUnassignedPixels > 0)
		{
			++taskSize;
			--numUnassignedPixels;
		}

		async_futures.emplace_back(std::async(std::launch::async, [=, this]
			{
				uint32_t pixelEnd = currPixelIdx + taskSize;
				for (uint32_t i = currPixelIdx; i < pixelEnd; ++i)
					RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);		
			}));

		currPixelIdx += taskSize;
	}

	//wait till all tasks are finished
	for (const std::future<void>& f : async_futures)
		f.wait(); //wait until next one

#elif defined(PARALLEL_FOR)
	//parallel
	concurrency::parallel_for(0u, numPixels, [=, this](int pixelIndex)
		{
			RenderPixel(pScene, pixelIndex, fov, aspectRatio, camera, lights, materials);
		});

#else
	//synchronous
	for (uint32_t pixel{0}; pixel < numPixels; ++pixel)
		RenderPixel(pScene, pixel, fov, aspectRatio, camera, lights, materials);
	
#endif

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void Renderer::CycleLightMode()
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

void Renderer::RenderPixel(Scene* pScene, uint32_t pixelIdx, float fov, float aspectRatio, const Camera& camera, 
							const std::vector<Light>& lights, const std::vector<Material*>& materials) const
{
	const int px{ int(pixelIdx) % m_Width };
	const int py{ int(pixelIdx) / m_Width };

	const float x{ float(((2 * (px + 0.5)) / m_Width) - 1) * aspectRatio * fov };
	const float y{ (1 - float((2 * (py + 0.5)) / m_Height)) * fov };
	
	const Vector3 rayDirection{ camera.cameraToWorld.TransformVector({ x, y, 1 }).Normalized() };

	const Ray viewRay{ camera.origin, rayDirection };
	ColorRGB finalColor{};
	HitRecord closestHit{};
	pScene->GetClosestHit(viewRay, closestHit);

	if (closestHit.didHit)
	{
		const auto& lights{ pScene->GetLights() }; //get all the lights of the scene

		for (const dae::Light& light : lights) //loop all lights
		{
			const Vector3 startPoint{ closestHit.origin + closestHit.normal * 0.01f }; //the point that just got hit
			const Vector3 direction{ LightUtils::GetDirectionToLight(light, startPoint) }; //vector from hit point to light
			Ray lightRay{ startPoint, direction }; //calculate the light ray
			lightRay.max = lightRay.direction.Normalize();
			const float lambertLaw{ Vector3::Dot(closestHit.normal, direction.Normalized()) };

			if (pScene->DoesHit(lightRay) && m_ShadowsEnabled)
				continue;

			const ColorRGB radiance{ LightUtils::GetRadiance(light, startPoint) };
			const ColorRGB brdf{ materials[closestHit.materialIndex]->Shade(closestHit, lightRay.direction, -rayDirection) };

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
