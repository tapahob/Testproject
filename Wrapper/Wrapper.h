// Wrapper.h
#pragma once
#pragma warning(disable:C4005)

namespace Wrapper {
	typedef float vector3[3];
	typedef float vector4[4];

	public ref class TEEngine
	{
	public:

		TEEngine(System::IntPtr window, int width, int height);
		~TEEngine();

		void Shoot(int x, int y);
		void Render();
	};


}
