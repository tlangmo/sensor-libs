#include "linalg.h"

void matrix_identity(Matrix44* mat) {
	memset(mat->m[0], 0, sizeof(float)*4);
	memset(mat->m[1], 0, sizeof(float)*4);
	memset(mat->m[2], 0, sizeof(float)*4);
	memset(mat->m[3], 0, sizeof(float)*4);
	mat->m[0][0] = mat->m[1][1] = mat->m[2][2] = mat->m[3][3] = 1.0f;
}
void matrix_from_a16(float* values, Matrix44* mat)
{
	memcpy((void*)mat, (void*)values, 16*sizeof(float));
}

void vector3_transform44(const Vector3& vIn, const Matrix44& mat,  Vector3* vOut) {
	vOut->m[0] = vIn.m[0] * mat.m[0][0] +  vIn.m[1] * mat.m[1][0] +  vIn.m[2] * mat.m[2][0] +  1.0f * mat.m[3][0];
	vOut->m[1] = vIn.m[0] * mat.m[0][1] +  vIn.m[1] * mat.m[1][1] +  vIn.m[2] * mat.m[2][1] +  1.0f * mat.m[3][1];
	vOut->m[2] = vIn.m[0] * mat.m[0][2] +  vIn.m[1] * mat.m[1][2] +  vIn.m[2] * mat.m[2][2] +  1.0f * mat.m[3][2];
}

void vector3_transform33(const Vector3& vIn, const Matrix44& mat,  Vector3* vOut) {
	vOut->m[0] = vIn.m[0] * mat.m[0][0] +  vIn.m[1] * mat.m[1][0] +  vIn.m[2] * mat.m[2][0];
	vOut->m[1] = vIn.m[0] * mat.m[0][1] +  vIn.m[1] * mat.m[1][1] +  vIn.m[2] * mat.m[2][1];
	vOut->m[2] = vIn.m[0] * mat.m[0][2] +  vIn.m[1] * mat.m[1][2] +  vIn.m[2] * mat.m[2][2];
}