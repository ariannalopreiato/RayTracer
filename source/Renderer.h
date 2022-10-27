#pragma once

#include <cstdint>
#include <vector>

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;
	struct Camera; 
	struct Light;
	class Material;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Render(Scene* pScene) const;
		bool SaveBufferToImage() const;
		void ToggleShadows() { m_ShadowsEnabled = !m_ShadowsEnabled; }
		void CycleLightMode();
		void RenderPixel(Scene* pScene, uint32_t pixelIdx, float fov, float aspectRatio, const Camera& camera, 
			const std::vector<Light>& lights, const std::vector<Material*>& materials) const;

	private:
		enum class LightingMode
		{
			ObservedArea, //lambert cosine law
			Radiance, //incident radiance
			BRDF, //scattering of the light
			Combined //observed area * radiance * BRDF
		};

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pBuffer{};
		uint32_t* m_pBufferPixels{};

		int m_Width{};
		int m_Height{};

		bool m_ShadowsEnabled{ true };
		LightingMode m_CurrentLightingMode{ LightingMode::Combined };
	};
}
