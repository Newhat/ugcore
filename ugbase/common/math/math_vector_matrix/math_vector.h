/**
 * \file math_vector.h
 */

//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m11 d13

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	This header defines common vector-types.
//	It is possible to completely avoid these vectors and to use your own.
//	Have a look at lgmath.h to see which typedefs have to be replaced.
//	You have to make sure that your vector-types specialize the
//	template methods defined in lgmath_vector_descriptor.

#ifndef __H__COMMON__MATH_VECTOR__
#define __H__COMMON__MATH_VECTOR__

#include <cstddef>
#include <iostream>
#include "../../ug_config.h"
#include "../../types.h"

namespace ug
{

////////////////////////////////////////////////////////////////////////
//	MathMathVector
///	a mathematical Vector with N entries.
template <std::size_t N, typename T = number> class MathVector;

/**
 * A mathematical Vector with N entries and static storage
 */
template <std::size_t N, typename T>
class MathVector
{
	public:
		typedef T value_type;
		typedef std::size_t size_type;
		static const std::size_t Size = N;

	public:
		MathVector() {}
		MathVector(const MathVector& v)	{assign(v);}

		// operations with other vectors
		MathVector& operator=  (const MathVector& v)
		{
			if(this != &v)
			{
				assign(v);
			}
			return *this;
		}
		MathVector& operator+= (const MathVector& v) {for(std::size_t i = 0; i < N; ++i) m_data[i] += v.coord(i);return *this;}
		MathVector& operator-= (const MathVector& v) {for(std::size_t i = 0; i < N; ++i) m_data[i] -= v.coord(i);return *this;}

		// operations with scalar
		MathVector& operator=  (const value_type& val) {for(std::size_t i = 0; i < N; ++i) m_data[i] =  val;return *this;}
		MathVector& operator+= (const value_type& val) {for(std::size_t i = 0; i < N; ++i) m_data[i] += val;return *this;}
		MathVector& operator-= (const value_type& val) {for(std::size_t i = 0; i < N; ++i) m_data[i] -= val;return *this;}
		MathVector& operator*= (const value_type& val) {for(std::size_t i = 0; i < N; ++i) m_data[i] *= val;return *this;}
		MathVector& operator/= (const value_type& val) {for(std::size_t i = 0; i < N; ++i) m_data[i] /= val;return *this;}

		// scalar product
		value_type operator* (const MathVector& v) const {value_type res = 0.0; for(std::size_t i = 0; i < N; ++i) res += m_data[i] * v.coord(i);return res;}

		inline std::size_t size() const {return N;}

		inline value_type& coord(size_t index)				{return m_data[index];}
		inline const value_type& coord(size_t index) const		{return m_data[index];}

		inline value_type& operator[](size_t index)				{return m_data[index];}
		inline const value_type& operator[](size_t index) const	{return m_data[index];}

	protected:
		value_type	m_data[N];

	protected:
		inline void assign(const MathVector<N>& v) {for(std::size_t i = 0; i < N; ++i) m_data[i] = v.coord(i);}

};

/**
 * A mathematical Vector with 1 entry and static storage
 */
template <typename T>
class MathVector<1, T>
{
	public:
		typedef std::size_t size_type;
		typedef T value_type;
		static const std::size_t Size = 1;

	public:
		MathVector()	{}
		MathVector(value_type x)
		{
			m_data = x;
		}
		MathVector(const MathVector<1, T>& v)	{assign(v);}

		// operations with other vectors
		MathVector& operator=  (const MathVector& v) {assign(v); return *this;}
		MathVector& operator+= (const MathVector& v) {m_data += v.x; return *this;}
		MathVector& operator-= (const MathVector& v) {m_data -= v.x; return *this;}

		// operations with scalar
		MathVector& operator=  (const value_type& val) {m_data =  val;return *this;}
		MathVector& operator+= (const value_type& val) {m_data += val;return *this;}
		MathVector& operator-= (const value_type& val) {m_data -= val;return *this;}
		MathVector& operator*= (const value_type& val) {m_data *= val;return *this;}
		MathVector& operator/= (const value_type& val) {m_data /= val;return *this;}

		// scalar product
		value_type operator* (const MathVector& v) const {return m_data * v.x;}

		inline std::size_t size() const								{return 1;}

		inline value_type& coord(std::size_t index)					{return m_data;}
		inline const value_type& coord(std::size_t index) const		{return m_data;}

		inline value_type& operator[](std::size_t index)				{return m_data;}
		inline const value_type& operator[](std::size_t index) const	{return m_data;}

	public:
		union
		{
			struct
			{
				value_type x;
			};

			value_type m_data;
		};

	protected:
		inline void assign(const MathVector<1, T>& v)	{m_data = v.x;}

};

/**
 * A mathematical Vector with 2 entries and static storage
 */
template <typename T>
class MathVector<2, T>
{
	public:
		typedef std::size_t size_type;
		typedef T value_type;
		static const std::size_t Size = 2;

	public:
		MathVector()	{}
		MathVector(value_type x, value_type y)
		{
			m_data[0] = x;
			m_data[1] = y;
		}
		MathVector(const MathVector<2,T>& v)	{assign(v);}

		// operations with other vectors
		MathVector& operator=  (const MathVector& v) {assign(v); return *this;}
		MathVector& operator+= (const MathVector& v) {for(std::size_t i = 0; i < 2; ++i) m_data[i] += v.coord(i);return *this;}
		MathVector& operator-= (const MathVector& v) {for(std::size_t i = 0; i < 2; ++i) m_data[i] -= v.coord(i);return *this;}

		// operations with scalar
		MathVector& operator=  (const value_type& val) {for(std::size_t i = 0; i < 2; ++i) m_data[i] =  val;return *this;}
		MathVector& operator+= (const value_type& val) {for(std::size_t i = 0; i < 2; ++i) m_data[i] += val;return *this;}
		MathVector& operator-= (const value_type& val) {for(std::size_t i = 0; i < 2; ++i) m_data[i] -= val;return *this;}
		MathVector& operator*= (const value_type& val) {for(std::size_t i = 0; i < 2; ++i) m_data[i] *= val;return *this;}
		MathVector& operator/= (const value_type& val) {for(std::size_t i = 0; i < 2; ++i) m_data[i] /= val;return *this;}

		// scalar product
		value_type operator* (const MathVector& v) const {value_type res = 0.0; for(std::size_t i = 0; i < 2; ++i) res += m_data[i] * v.coord(i);return res;}

		inline std::size_t size() const								{return 2;}

		inline value_type& coord(std::size_t index)					{return m_data[index];}
		inline const value_type& coord(std::size_t index) const		{return m_data[index];}

		inline value_type& operator[](std::size_t index)				{return m_data[index];}
		inline const value_type& operator[](std::size_t index) const	{return m_data[index];}

	public:
		union
		{
			struct
			{
				value_type x;
				value_type y;
			};

			value_type m_data[2];
		};

	protected:
		inline void assign(const MathVector<2,T>& v)	{m_data[0] = v.coord(0);m_data[1] = v.coord(1);}

};

/**
 * A mathematical Vector with 3 entries and static storage
 */
template <typename T>
class MathVector<3, T>
{
	public:
		typedef std::size_t size_type;
		typedef T value_type;
		static const std::size_t Size = 3;

	public:
		MathVector()	{}
		MathVector(value_type x, value_type y, value_type z)
		{
			m_data[0] = x;
			m_data[1] = y;
			m_data[2] = z;
		}
		MathVector(const MathVector<3,T>& v)	{assign(v);}

		// operations with other vectors
		MathVector& operator=  (const MathVector& v) {assign(v); return *this;}
		MathVector& operator+= (const MathVector& v) {for(std::size_t i = 0; i < 3; ++i) m_data[i] += v.coord(i);return *this;}
		MathVector& operator-= (const MathVector& v) {for(std::size_t i = 0; i < 3; ++i) m_data[i] -= v.coord(i);return *this;}

		// operations with scalar
		MathVector& operator=  (const value_type& val) {for(std::size_t i = 0; i < 3; ++i) m_data[i] =  val;return *this;}
		MathVector& operator+= (const value_type& val) {for(std::size_t i = 0; i < 3; ++i) m_data[i] += val;return *this;}
		MathVector& operator-= (const value_type& val) {for(std::size_t i = 0; i < 3; ++i) m_data[i] -= val;return *this;}
		MathVector& operator*= (const value_type& val) {for(std::size_t i = 0; i < 3; ++i) m_data[i] *= val;return *this;}
		MathVector& operator/= (const value_type& val) {for(std::size_t i = 0; i < 3; ++i) m_data[i] /= val;return *this;}

		// scalar product
		value_type operator* (const MathVector& v) const {value_type res = 0.0; for(std::size_t i = 0; i < 3; ++i) res += m_data[i] * v.coord(i);return res;}

		inline std::size_t size() const										{return 3;}

		inline value_type& coord(std::size_t index)					{return m_data[index];}
		inline const value_type& coord(std::size_t index) const		{return m_data[index];}

		inline value_type& operator[](std::size_t index)				{return m_data[index];}
		inline const value_type& operator[](std::size_t index) const	{return m_data[index];}

	public:
		union
		{
			struct
			{
				value_type x;
				value_type y;
				value_type z;
			};

			value_type m_data[3];
		};
	protected:
		inline void assign(const MathVector<3,T>& v)	{m_data[0] = v.coord(0);
												 m_data[1] = v.coord(1);
												 m_data[2] = v.coord(2);}

};

/**
 * A mathematical Vector with 4 entries and static storage
 */
template <typename T>
class MathVector<4, T>
{
	public:
		typedef std::size_t size_type;
		typedef T value_type;
		static const std::size_t Size = 4;

	public:
		MathVector()	{}
		MathVector(value_type x, value_type y, value_type z, value_type w)
		{
			m_data[0] = x;
			m_data[1] = y;
			m_data[2] = z;
			m_data[3] = w;
		}
		MathVector(const MathVector<4,T>& v)	{assign(v);}

		// operations with other vectors
		MathVector& operator=  (const MathVector& v) {assign(v); return *this;}
		MathVector& operator+= (const MathVector& v) {for(std::size_t i = 0; i < 4; ++i) m_data[i] += v.coord(i);return *this;}
		MathVector& operator-= (const MathVector& v) {for(std::size_t i = 0; i < 4; ++i) m_data[i] -= v.coord(i);return *this;}

		// operations with scalar
		MathVector& operator=  (const value_type& val) {for(std::size_t i = 0; i < 4; ++i) m_data[i] =  val;return *this;}
		MathVector& operator+= (const value_type& val) {for(std::size_t i = 0; i < 4; ++i) m_data[i] += val;return *this;}
		MathVector& operator-= (const value_type& val) {for(std::size_t i = 0; i < 4; ++i) m_data[i] -= val;return *this;}
		MathVector& operator*= (const value_type& val) {for(std::size_t i = 0; i < 4; ++i) m_data[i] *= val;return *this;}
		MathVector& operator/= (const value_type& val) {for(std::size_t i = 0; i < 4; ++i) m_data[i] /= val;return *this;}

		// scalar product
		value_type operator* (const MathVector& v) const {value_type res = 0.0; for(std::size_t i = 0; i < 4; ++i) res += m_data[i] * v.coord(i);return res;}

		inline std::size_t size() const									{return 4;}

		inline value_type& coord(std::size_t index)					{return m_data[index];}
		inline const value_type& coord(std::size_t index) const		{return m_data[index];}

		inline value_type& operator[](std::size_t index)				{return m_data[index];}
		inline const value_type& operator[](std::size_t index) const	{return m_data[index];}

	public:
		union
		{
			struct
			{
				value_type x;
				value_type y;
				value_type z;
				value_type w;
			};

			value_type m_data[4];
		};

	protected:
		inline void assign(const MathVector<4,T>& v)	{m_data[0] = v.coord(0);
												 m_data[1] = v.coord(1);
												 m_data[2] = v.coord(2);
												 m_data[3] = v.coord(3);}

};

template <std::size_t N, typename T>
bool operator== (const MathVector<N,T>& v, const MathVector<N,T>& w)
{
	for(std::size_t i = 0; i < N; ++i)
	{
		if(v[i] != w[i]) return false;
	}
	return true;
}

template <std::size_t N, typename T>
bool operator!= (const MathVector<N,T>& v, const MathVector<N,T>& w)
{
	return !(v == w);
}

template <std::size_t N, typename T>
std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<N,T>& v)
{
	for(std::size_t i = 0; i < N; ++i)
		outStream << "[" << i << "]: " << v.coord(i) << std::endl;
	return outStream;
}

template <typename T>
std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<1,T>& v)
{
	outStream << "(" << v[0] << ")";
	return outStream;
}
template <typename T>
std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<2,T>& v)
{
	outStream << "("<<v[0]<<", "<<v[1]<<")";
	return outStream;
}
template <typename T>
std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<3,T>& v)
{
	outStream << "("<<v[0]<<", "<<v[1]<<", "<<v[2]<<")";
	return outStream;
}
template <typename T>
std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<4,T>& v)
{
	outStream << "("<<v[0]<<", "<<v[1]<<", "<<v[2]<<", "<<v[3]<<")";
	return outStream;
}

UG_API std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<1>& v);
UG_API std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<2>& v);
UG_API std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<3>& v);
UG_API std::ostream& operator<< (std::ostream& outStream, const ug::MathVector<4>& v);

}//	end of namespace


#endif /* __H__COMMON__MATH_MathVector__ */
