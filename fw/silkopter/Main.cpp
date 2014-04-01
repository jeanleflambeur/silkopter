#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
//#include <chrono>
#include <qmath.h>
#include <board/board.h>
#include <util/format.h>
#include "GS/GS.h"

__extension__ typedef int __guard __attribute__((mode (__DI__))); 

extern "C" int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
extern "C" void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
extern "C" void __cxa_guard_abort (__guard *) {};
extern "C" void __cxa_pure_virtual() { while (1); }

int main(void)
{
	sei();
	
	board::Init_Params params;
	board::init(params);
	
	//board::s_uarts[0].begin(115200);
	debug::init(&board::get_gs_full_uart());
	
	board::pwm_out::set_all_enabled(true);
	
	util::FString<128> str;

	silk::UAV uav(silk::Motor_Mixer::Type::X, 4, 0.3f);
	silk::GS gs(uav, board::get_gs_full_uart());

// 	auto last_fps = board::clock::now_ms();
// 	uint32_t fps = 0;
// 	uint32_t crt_fps = 0;
	
    while(1)
    {
		auto start = board::clock::now_us();
		//auto start = now;

		uav.process();

		
// 		{
// 			float max_error = 0.f;
// 			float avg_error = 0.f;
// 			size_t range = 100;
// 			const float range_inv = 1.f / range;
// 			for (volatile size_t i = 0; i < range; i++)
// 			{
// 				float angle = ((i * range_inv) * math::anglef::_2pi.radians);
// 				float sin, cos;
// 				math::sin_cos(angle, sin, cos);
// 				float fsin, fcos;
// 				math::sin_cos<float, math::fast>(angle, fsin, fcos);
// 				float serror = math::abs(sin - fsin);
// 				float cerror = math::abs(cos - fcos);
// 				max_error = math::max(max_error, serror);
// 				max_error = math::max(max_error, cerror);
// //				avg_error += serror;
// 			}
// 			avg_error /= float(range);
// 			PRINT("\nmax_error = {0}, avg_error = {1}", max_error, avg_error);
// 			float t = 0, t1 = 0;
// 			{
// 				TIMED_BLOCK();
// 				for (volatile size_t i = 0; i < range; i++)
// 				{
// 					float angle = i * math::anglef::_2pi.radians * range_inv;
// 					t += math::sin(angle);
// 				}
// 			}
// 			{
// 				TIMED_BLOCK();
// 				for (volatile size_t i = 0; i < range; i++)
// 				{
// 					float angle = i * math::anglef::_2pi.radians * range_inv;
// 					t1 += math::sin<float, math::fast>(angle);
// 				}
// 			}
// 			PRINT("\n{0} == {1}", t, t1);
// 		}
// 
		
		//the extra time - run the ground station connection
		//for up to 3ms
		{
			auto duration = board::clock::now_us() - start;
			if (duration < chrono::micros(5000))
			{
				gs.process(chrono::micros(5000) - duration);
			}
		}
   }
}