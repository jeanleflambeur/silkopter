#pragma once

#include <stdint.h>
#include <stddef.h>
#include <algorithm>
#include <math.h>
#include "util/FString.h"
#include "chrono.h"

namespace util
{
	namespace formatting
	{
		struct Placeholder
		{
			Placeholder() : alignment(0), precision(0), base_case(0), base(10), filler(' ') {}
			int8_t alignment;
			int8_t precision;
			uint8_t base_case : 1; //lower == 0 / upper case == 1
			uint8_t base : 7;
			char filler;
		};

		namespace detail
		{
			extern const double s_pow_10[];
			extern const char s_digits[201];

			template<class T>
			void decompose_digits(char* buf, int8_t length, T uvalue)
			{
				auto off_end = length - 1;
				while (uvalue >= 100)
				{
					const uint32_t i = (uvalue % 100) * 2;
					uvalue /= 100;
					buf[off_end] = detail::s_digits[i + 1];
					buf[off_end - 1] = detail::s_digits[i];
					off_end -= 2;
				}

				// Handle last 1-2 digits.
				if (uvalue < 10)
				{
					buf[off_end] = '0' + static_cast<char>(uvalue);
				}
				else
				{
					const uint32_t i = static_cast<uint32_t>(uvalue)* 2;
					buf[off_end] = detail::s_digits[i + 1];
					buf[off_end - 1] = detail::s_digits[i];
				}
			}


			//////////////////////////////////////////////////////////////////////////

			auto get_base_10_digit_count(uint64_t v)->int8_t;
			auto get_base_10_digit_count(uint32_t v)->int8_t;
			auto get_base_10_digit_count(uint16_t v)->int8_t;
			auto get_base_10_digit_count(uint8_t v)->int8_t;
		}

		//////////////////////////////////////////////////////////////////////////

		//this class is an adapter that is able to iterate over an input string
		template<class Format_String> struct Format_String_Adapter
		{
		public:
			typedef typename Format_String::value_type value_type;
			typedef typename Format_String::const_iterator iterator;
			Format_String_Adapter(Format_String const& fmt) : m_it(fmt.begin()), m_end(fmt.end()) {}
			auto is_done() const -> bool { return m_it == m_end; }
			auto get() -> value_type { return *m_it; }
			auto get_and_advance() -> value_type { return *m_it++; }
		private:
			iterator m_it;
			iterator m_end;
		};
		template<> struct Format_String_Adapter<char const*>
		{
		public:
			typedef char value_type;
			typedef char const* iterator;
			Format_String_Adapter(char const* fmt) : m_it(fmt) {}
			auto is_done() const -> bool { return *m_it == 0; }
			auto get() -> value_type { return *m_it; }
			auto get_and_advance() -> value_type { return *m_it++; }
		private:
			const char* m_it;
		};

		//this is needed for const char[N]
		template<typename T, size_t N> struct Format_String_Adapter<T[N]>
		{
		public:
			typedef T value_type;
			typedef T const* iterator;
			Format_String_Adapter(T const* fmt) : m_it(fmt) {}
			auto is_done() const -> bool { return *m_it == 0; }
			auto get() -> value_type { return *m_it; }
			auto get_and_advance() -> value_type { return *m_it++; }
		private:
			T const* m_it;
		};
		
		template<> struct Format_String_Adapter<Flash_String>
		{
		public:
			typedef Flash_String::value_type value_type;
			typedef Flash_String::const_iterator iterator;
			Format_String_Adapter(Flash_String const& fmt) : m_it(fmt.begin()) {}
			auto is_done() const -> bool { return *m_it == 0; }
			auto get() -> value_type { return *m_it; }
			auto get_and_advance() -> value_type { auto c = *m_it; m_it++; return c; }
		private:
			Flash_String::const_iterator m_it;
		};

		template<class T> struct Dst_Adapter
		{
		public:
			Dst_Adapter(T& dst) : m_dst(dst) {}
			void append(char ch) { m_dst.push_back(ch); }
			void append(char const* start, char const* end) { m_dst.append(start, end); }
			void clear() { m_dst.clear(); }
		private:
			T& m_dst;
		};

		template<class Dst_Adapter, class Placeholder, size_t SIZE>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, FString<SIZE> const& p)
		{
			dst.append(p.begin(), p.end());
		}
		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, Flash_String const& p)
		{
			for (auto it = p.begin(); *it != 0; ++it)
			{
				dst.append(*it);
			}
		}
		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, const char* p)
		{
			if (p)
			{
				for (; *p != 0; ++p)
				{
					dst.append(*p);
				}
			}
		}
		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, char p)
		{
			dst.append(p);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, int8_t value)
		{
			char buf[8];
			if (ph.base == 16)
			{
				sprintf(buf, ph.base_case == 0 ? "%x" : "%X", value);
				format_string(dst, Placeholder(), buf);
				return;
			}

			// Take care of sign.
			uint8_t uvalue = (value < 0) ? -value : value;
			auto length = util::formatting::detail::get_base_10_digit_count(uvalue);
			auto aligned_length = std::max(length, ph.alignment);
			if (value < 0)
			{
				//to make space for the zero
				aligned_length--;
			}
			for (int8_t i = length; i < aligned_length; i++)
			{
				dst.append(ph.filler);
			}
			if (value < 0)
			{
				dst.append('-');
			}
			detail::decompose_digits(buf, length, uvalue);
			dst.append(buf, buf + length);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, uint8_t value)
		{
			char buf[8];
			if (ph.base == 16)
			{
				sprintf(buf, ph.base_case == 0 ? "%x" : "%X", value);
				format_string(dst, Placeholder(), buf);
				return;
			}
			auto uvalue = value;
			auto length = util::formatting::detail::get_base_10_digit_count(uvalue);
			auto aligned_length = std::max(length, ph.alignment);
			for (int8_t i = length; i < aligned_length; i++)
			{
				dst.append(ph.filler);
			}
			detail::decompose_digits(buf, length, uvalue);
			dst.append(buf, buf + length);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, int16_t value)
		{
			char buf[8];
			if (ph.base == 16)
			{
				sprintf(buf, ph.base_case == 0 ? "%x" : "%X", value);
				format_string(dst, Placeholder(), buf);
				return;
			}
			// Take care of sign.
			uint16_t uvalue = (value < 0) ? -value : value;
			auto length = util::formatting::detail::get_base_10_digit_count(uvalue);
			auto aligned_length = std::max(length, ph.alignment);
			if (value < 0)
			{
				//to make space for the zero
				aligned_length--;
			}
			for (int8_t i = length; i < aligned_length; i++)
			{
				dst.append(ph.filler);
			}
			if (value < 0)
			{
				dst.append('-');
			}
			detail::decompose_digits(buf, length, uvalue);
			dst.append(buf, buf + length);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, uint16_t value)
		{
			char buf[8];
			if (ph.base == 16)
			{
				sprintf(buf, ph.base_case == 0 ? "%x" : "%X", value);
				format_string(dst, Placeholder(), buf);
				return;
			}
			auto uvalue = value;
			auto length = util::formatting::detail::get_base_10_digit_count(uvalue);
			auto aligned_length = std::max(length, ph.alignment);
			for (int8_t i = length; i < aligned_length; i++)
			{
				dst.append(ph.filler);
			}
			detail::decompose_digits(buf, length, uvalue);
			dst.append(buf, buf + length);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, int32_t value)
		{
			//if (ph.base == 16)
			//{
			//char buf[32];
			//sprintf(buf, ph.base_case == 0 ? "%x" : "%X", value);
			//format_string(dst, Placeholder(), buf);
			//return;
			//}

			// Take care of sign.
			uint32_t uvalue = (value < 0) ? -value : value;
			auto length = util::formatting::detail::get_base_10_digit_count(uvalue);
			auto aligned_length = std::max(length, ph.alignment);
			if (value < 0)
			{
				//to make space for the zero
				aligned_length--;
			}
			for (int8_t i = length; i < aligned_length; i++)
			{
				dst.append(ph.filler);
			}
			if (value < 0)
			{
				dst.append('-');
			}
			char buf[16];
			detail::decompose_digits(buf, length, uvalue);
			dst.append(buf, buf + length);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, uint32_t value)
		{
			// 		if (ph.base == 16)
			// 		{
			// 			char buf[32];
			// 			sprintf(buf, ph.baseCase == 0 ? "%x" : "%X", value);
			// 			format_string(dst, Placeholder(), buf);
			// 			return;
			// 		}

			auto uvalue = value;
			auto length = util::formatting::detail::get_base_10_digit_count(uvalue);
			auto aligned_length = std::max(length, ph.alignment);
			for (int8_t i = length; i < aligned_length; i++)
			{
				dst.append(ph.filler);
			}
			char buf[16];
			detail::decompose_digits(buf, length, uvalue);
			dst.append(buf, buf + length);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, int64_t value)
		{
			// Take care of sign.
			uint64_t uvalue = (value < 0) ? -value : value;
			auto length = util::formatting::detail::get_base_10_digit_count(uvalue);
			auto aligned_length = std::max(length, ph.alignment);
			if (value < 0)
			{
				//to make space for the zero
				aligned_length--;
			}
			for (int8_t i = length; i < aligned_length; i++)
			{
				dst.append(ph.filler);
			}
			if (value < 0)
			{
				dst.append('-');
			}
			char buf[32];
			detail::decompose_digits(buf, length, uvalue);
			dst.append(buf, buf + length);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, uint64_t value)
		{
			auto uvalue = value;
			auto length = util::formatting::detail::get_base_10_digit_count(uvalue);
			auto aligned_length = std::max(length, ph.alignment);
			for (int8_t i = length; i < aligned_length; i++)
			{
				dst.append(ph.filler);
			}
			char buf[32];
			detail::decompose_digits(buf, length, uvalue);
			dst.append(buf, buf + length);
		}

		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, double value)
		{
			if (!(value == value))
			{
				// Handle the 'width' parameter.
				format_string(dst, ph, "nan");
				return;
			}

			//to format the parts without alignment or precision
			Placeholder ph2;
			ph2.filler = '0';
			ph2.alignment = 6;

			//the double is formatted as a series of integers
			int32_t whole_parts[16]; //each will hold 9 decimals
			int8_t whole_parts_count = 0;

			int32_t frac_parts[16];
			uint8_t last_frac_part_alignment = 6;
			int8_t frac_parts_count = 0;

			uint8_t precision = ph.precision;
			if (precision == 0)
			{
				precision = 5;
			}
			else if (precision > 9)
			{
				precision = 9;
			}

			bool is_negative = value < 0.0;
			value = is_negative ? -value : value;

			double whole = floorf(value);
			double frac = value - whole;

			//store the whole parts
			double v = whole;
			if (v > 0.0f)
			{
				do
				{
					whole_parts[whole_parts_count++] = (int32_t)fmod(v, 1000000);
					if (v < 1000000)
					{
						break;
					}
					v /= 1000000;
				} while (true);
			}

			//store the fract parts
			//we transform the frac part like this (For a precision of 5):
			//	1.	0.0004272
			//	2. 	0.0004272 * 100000 == 42
			v = floor(frac * detail::s_pow_10[precision] + 0.5f);
			if (v > 0.0f)
			{
				//	3.	42 + 100000 = 100042
				v += detail::s_pow_10[precision];
				do
				{
					int32_t r = (int32_t)fmod(v, 1000000);
					if (r > 0)
					{
						if (frac_parts_count == 0)
						{
							last_frac_part_alignment = 6;
							//remove trailing zeros
							int32_t r2 = r / 10;
							while (r2 * 10 == r)
							{
								r = r2;
								r2 /= 10;
								last_frac_part_alignment--;
							}
						}
						frac_parts[frac_parts_count++] = r;
					}
					if (v < 1000000)
					{
						break;
					}
					v /= 1000000;
				} while (true);
			}

			if (whole_parts_count > 0)
			{
				if (is_negative)
				{
					whole_parts[whole_parts_count - 1] *= -1;
				}

				//first part doesn't need '0' padding
				ph2.alignment = 0;
				format_string(dst, ph2, whole_parts[whole_parts_count - 1]);

				//next parts are 9 digits each
				ph2.alignment = 9;
				for (int8_t i = whole_parts_count - 2; i >= 0; i--)
				{
					format_string(dst, ph2, whole_parts[i]);
				}
			}
			else
			{
				format_string(dst, ph, is_negative ? "-0" : "0");
			}

			if (frac_parts_count > 0)
			{
				FString<16> buf;
				//auto decimal_point_off = dst.size();
				//parseToString(dst, off, Placeholder(), '.');

				//first part - no alignment needed
				ph2.alignment = 0;
				format_string(buf, ph2, frac_parts[frac_parts_count - 1]);

				//middle part - alignment is 9
				ph2.alignment = 9;
				for (int8_t i = frac_parts_count - 2; i >= 1; i--)
				{
					format_string(buf, ph2, frac_parts[i]);
				}

				if (frac_parts_count > 1)
				{
					//the last part might be cut as we remove zeroes from it
					ph2.alignment = last_frac_part_alignment;
					format_string(buf, ph2, frac_parts[0]);
				}

				//overwrite the 1 from the frac part with the decimal point
				buf[0] = '.';
				
				dst.append(buf.data(), buf.data() + buf.size());
			}
		}

		template<class Dst_Adapter, class Placeholder, class P0, class P1>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, std::pair<P0, P1> const& pair)
		{
			dst.append('<');
			format_string(dst, ph, pair.first);
			dst.append(',');
			dst.append(' ');
			format_string(dst, ph, pair.second);
			dst.append('>');
		}

		// 	template<class Dst_Adapter, class Placeholder, class rep, class period>
		// 	void format_string(Dst_Adapter& dst, auto& off, Placeholder const& ph, std::chrono::duration<rep, period> const& duration)
		// 	{
		// 		using namespace std::chrono;
		// 
		// 		auto d = duration;
		// 		auto h = duration_cast<hours>(d);
		// 		d -= duration_cast<std::chrono::duration<rep, period>>(h);
		// 
		// 		auto m = duration_cast<minutes>(d);
		// 		d -= duration_cast<std::chrono::duration<rep, period>>(m);
		// 
		// 		auto s = duration_cast<seconds>(d);
		// 		d -= duration_cast<std::chrono::duration<rep, period>>(s);
		// 
		// 		auto ms = duration_cast<milliseconds>(d);
		// 
		// 		Placeholder ph2 = ph;
		// 		ph2.filler = '0';
		// 		ph2.alignment = 2;
		// 		format_string(dst, off, ph2, h.count());
		// 		format_string(dst, off, ph, ':');
		// 		format_string(dst, off, ph2, (int32_t)m.count());
		// 		format_string(dst, off, ph, ':');
		// 		format_string(dst, off, ph2, (int32_t)s.count());
		// 		format_string(dst, off, ph, '.');
		// 		format_string(dst, off, ph, (int32_t)ms.count());
		// 	}
		// 
		template<class Dst_Adapter, class Placeholder, class Rep, class Duration>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, chrono::time<Rep, Duration> const& time)
		{
			format_string(dst, ph, time.time_since_epoch());
		}
		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, chrono::micros us)
		{
			format_string(dst, ph, us.count);
			format_string(dst, ph, "us");
		}
		template<class Dst_Adapter, class Placeholder>
		void format_string(Dst_Adapter& dst, Placeholder const& ph, chrono::millis ms)
		{
			format_string(dst, ph, ms.count);
			format_string(dst, ph, "ms");
		}

		template<class Dst_Adapter, class P>
		struct Argument_Parser
		{
			void execute(Dst_Adapter& dst, Placeholder const& ph, P const& p)
			{
				format_string(dst, ph, p);
			}
		};
		
		template<class Dst_Adapter, class Format_String_Adapter>
		bool _copy_until_placeholder(Dst_Adapter& dst, Format_String_Adapter& fmt_adapter) // base function
		{
			while (!fmt_adapter.is_done())
			{
				auto ch = fmt_adapter.get_and_advance();
				if (ch != '{')
				{
					dst.append(ch);
					continue;
				}

				ch = fmt_adapter.get();
				if (ch == '{')
				{
					dst.append('{');
					dst.append('{');
					fmt_adapter.get_and_advance();
					continue;
				}

				return true;
			} 
			return false;
		}
		
		template<class Format_String_Adapter>
		bool _read_placeholder(util::formatting::Placeholder& ph, Format_String_Adapter& fmt_adapter) // base function
		{
			auto ch = fmt_adapter.get_and_advance();
			if (ch == 'x')
			{
				ph.base = 16;
				ch = fmt_adapter.get_and_advance();
			}
			else if (ch == 'X')
			{
				ph.base_case = 1;
				ph.base = 16;
				ch = fmt_adapter.get_and_advance();
			}
			else if (ch == '.')
			{
				ch = fmt_adapter.get_and_advance();
				if (ch < '0' || ch > '9')
				{
					return false;
				}
				ph.precision = ch - '0';
				ch = fmt_adapter.get_and_advance();
			}
			return (ch == '}');
		}

		template<class Dst_Adapter, class Format_String_Adapter>
		void _format(Dst_Adapter& dst, Format_String_Adapter& fmt_adapter) // base function
		{
			bool found_placeholder = _copy_until_placeholder(dst, fmt_adapter);
			if (found_placeholder)
			{
				//assert(0);
			}
		}

		template<class Dst_Adapter, class Format_String_Adapter, typename P, typename... Params>
		void _format(Dst_Adapter& dst, Format_String_Adapter& fmt_adapter, P const& p, Params... params) // recursive variadic function
		{
			while (!fmt_adapter.is_done())
			{
				bool found_placeholder = _copy_until_placeholder(dst, fmt_adapter);
				if (!found_placeholder)
				{
					return;
				}
				util::formatting::Placeholder ph;
				bool ok = _read_placeholder(ph, fmt_adapter);
				if (!ok)
				{
					//assert(0);
					return;
				}

				util::formatting::Argument_Parser<Dst_Adapter, P>().execute(dst, ph, p);
				_format(dst, fmt_adapter, params...);
			}
		}
	}




	template<class Dst, class Format_String, typename... Params>
	Dst& format(Dst& dst, Format_String const& fmt, Params... params) // recursive variadic function
	{
		util::formatting::Dst_Adapter<Dst> dst_adapter(dst);
		dst_adapter.clear();
		
		util::formatting::Format_String_Adapter<Format_String> fmt_adapter(fmt);
		if (fmt_adapter.is_done())
		{
			return dst;
		}

		formatting::_format(dst_adapter, fmt_adapter, params...);
		return dst;
	}

}
