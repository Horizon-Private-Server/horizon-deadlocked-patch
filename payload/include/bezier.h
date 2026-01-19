#ifndef BEZIER_H
#define BEZIER_H

#include <tamtypes.h>
#include <libdl/math.h>
#include <libdl/math3d.h>

typedef struct BezierPoint
{
	VECTOR HandleIn;
	VECTOR ControlPoint;
	VECTOR HandleOut;
} BezierPoint_t;

void bezierGetPosition(VECTOR out, BezierPoint_t * a, BezierPoint_t * b, float t);
void bezierGetTangent(VECTOR out, BezierPoint_t * a, BezierPoint_t * b, float t);
void bezierGetNormal(VECTOR out, BezierPoint_t * a, BezierPoint_t * b, float t);
float bezierGetLength(BezierPoint_t * a, BezierPoint_t * b, float tA, float tB);
float bezierMove(float* t, BezierPoint_t * a, BezierPoint_t * b, float distance);
float bezierMovePath(float* time, int* index, BezierPoint_t* vertices, int vertexCount, float distance);
float bezierGetClosestPointOnPath(VECTOR out, VECTOR position, BezierPoint_t* vertices, float* segmentLengths, int vertexCount);

#endif // BEZIER_H
