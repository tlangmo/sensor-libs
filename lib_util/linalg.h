#pragma once

#include <memory>
#include <cstring>

struct Matrix44
{
	float m[4][4];
	const float& operator[](size_t idx) const{
		return m[idx/4][idx%4];
	}
};

struct Vector3 {
	Vector3() {
		memset(m,0,sizeof(float)*3);
	}

	Vector3(const Vector3& rhs) {
		m[0] = rhs.m[0];
		m[1] = rhs.m[1];
		m[2] = rhs.m[2];

	}

	Vector3(float x, float y, float z) {
		m[0] = x;
		m[1] = y;
		m[2] = z;
	}

	const float& operator[](size_t idx) const {
		return m[idx];
	}

	float& operator[](size_t idx) {
		return m[idx];
	}

	Vector3 operator+(const Vector3& rhs) const{
		Vector3 res(m[0],m[1],m[2]);
		res.m[0] += rhs.m[0];
		res.m[1] += rhs.m[1];
		res.m[2] += rhs.m[2];
		return res;
	}

	Vector3 operator-(const Vector3& rhs) {
		Vector3 res(-rhs.m[0],-rhs.m[1],-rhs.m[2]);
		return *this + res;
	}


	float dot(const Vector3& rhs) {
		return m[0]*rhs.m[0] + m[1]*rhs.m[1] + m[2]+rhs.m[2];
	}
	float m[3];

};

inline
Vector3 operator*(const Vector3& lhs, float scalar) {
	Vector3 res(lhs.m[0]*scalar,lhs.m[1]*scalar,lhs.m[2]*scalar);
	return  res;
}

inline
Vector3 operator*(float scalar, const Vector3& rhs ) {
	return  rhs*scalar;
}

inline
Vector3 operator/(const Vector3& rhs, float scalar) {
	return rhs * (1.0f/scalar);
}


void matrix_identity(Matrix44* mat);
void matrix_from_a16(float* values, Matrix44* mat);
void vector3_transform44(const Vector3& vIn, const Matrix44& mat,  Vector3* vOut) ;
void vector3_transform33(const Vector3& vIn, const Matrix44& mat,  Vector3* vOut);

//uint32_t calculate_hash(const uint8_t* msg, size_t len);