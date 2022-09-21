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

	for (int px{}; px < m_Width; ++px)
	{
		float x = (((2 * (px + 0.5)) / m_Width) - 1) * (m_Width / m_Height);

		for (int py{}; py < m_Height; ++py)
		{
			float y = 1 - ((2 * (py + 0.5)) / m_Height);

			//float gradient = px / static_cast<float>(m_Width);
			//gradient += py / static_cast<float>(m_Width);
			//gradient /= 2.0f;
			//ColorRGB finalColor{ gradient, gradient, gradient };

			/*Vector3 rayDirection{ x, y, 1 };
			Ray hitRay{ {0,0,0}, rayDirection };
			ColorRGB finalColor{ rayDirection.x, rayDirection.y, rayDirection.z };*/

			Vector3 rayDirection{ x, y, 1 };
			Ray hitRay{ {0,0,0}, rayDirection };
			ColorRGB finalColor{};
			HitRecord closestHit{};
			Sphere testSphere{ {0.f, 0.f, 100.f}, 50.f, 0 };
			GeometryUtils::HitTest_Sphere(testSphere, hitRay, closestHit);

			if (closestHit.didHit)
			{
				finalColor = materials[closestHit.materialIndex]->Shade();
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
