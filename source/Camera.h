#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};

		float moveFactor{ 0.f };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld()
		{
			right = Vector3::Cross({0, 1, 0}, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();
			cameraToWorld = { right, up, forward, origin };
			return cameraToWorld;
		}

		void Update(Timer* pTimer)
		{
			moveFactor = 1.f;

			const float deltaTime{ pTimer->GetElapsed() };
			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP])
				origin.z += moveFactor;
			if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN])
				origin.z -= moveFactor;
			if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT])
				origin.x -= moveFactor;
			if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT])
				origin.x += moveFactor;

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
			if ((mouseState & SDL_BUTTON_LMASK) != 0)
			{
				if (pKeyboardState[SDL_SCANCODE_LSHIFT] || pKeyboardState[SDL_SCANCODE_RSHIFT])
					moveFactor += 4.f;

				if ((mouseState & SDL_BUTTON_RMASK) != 0)
				{
					if (mouseY < 0)
						origin.y += moveFactor;
					if (mouseY > 0)
						origin.y -= moveFactor;				
				}
				else
				{
					if (mouseY < 0)
						origin.z += moveFactor;
					if (mouseY > 0)
						origin.z -= moveFactor;

					totalYaw -= mouseX * deltaTime;
				}
			}
			else 
			{
				if ((mouseState & SDL_BUTTON_RMASK) != 0)
				{
					totalPitch -= mouseY * deltaTime;
					totalYaw -= mouseX * deltaTime;
				}
			}
			const Matrix rotation{ Matrix::CreateRotation(totalPitch, totalYaw, 0.f) };
			forward = rotation.TransformVector(Vector3::UnitZ);
			forward.Normalize();
		}
	};
}
