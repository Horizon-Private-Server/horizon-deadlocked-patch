#include "include/bezier.h"

//--------------------------------------------------------------------------
void bezierGetPosition(VECTOR out, BezierPoint_t * a, BezierPoint_t * b, float t)
{
	VECTOR temp;
	float iT = 1 - t;

	if (t <= 0) {
		vector_copy(out, a->ControlPoint);
	} else if (t >= 1) {
		vector_copy(out, b->ControlPoint);
	} else {
		vector_scale(out, a->ControlPoint, powf(iT, 3));
		vector_scale(temp, a->HandleOut, 3 * t * powf(iT, 2));
		vector_add(out, out, temp);
		vector_scale(temp, b->HandleIn, 3 * powf(t, 2) * iT);
		vector_add(out, out, temp);
		vector_scale(temp, b->ControlPoint, powf(t, 3));
		vector_add(out, out, temp);
	}
}

//--------------------------------------------------------------------------
void bezierGetTangent(VECTOR out, BezierPoint_t * a, BezierPoint_t * b, float t)
{
	VECTOR temp;
	const float delta = 0.001;
	const float invDelta = 1 / delta;

	if (t >= 1)
	{
		bezierGetPosition(temp, a, b, t - delta);
		bezierGetPosition(out, a, b, t);
	}
	else
	{
		bezierGetPosition(temp, a, b, t);
		bezierGetPosition(out, a, b, t + delta);
	}

	vector_subtract(out, out, temp);
	vector_scale(out, out, invDelta);
	vector_normalize(out, out);
}

//--------------------------------------------------------------------------
void bezierGetNormal(VECTOR out, BezierPoint_t * a, BezierPoint_t * b, float t)
{
	VECTOR tangent;
	VECTOR up = {0,0,1,0};
	VECTOR right = {-1,0,0,0};

	bezierGetTangent(tangent, a, b, t);

	vector_outerproduct(out, tangent, right);
	if (vector_length(out) == 0)
		vector_outerproduct(out, tangent, up);
		
	vector_normalize(out, out);
}

//--------------------------------------------------------------------------
float bezierGetLength(BezierPoint_t * a, BezierPoint_t * b, float tA, float tB)
{
	VECTOR lastPos, curPos, delta;
	vector_subtract(delta, a->ControlPoint, b->ControlPoint);
	float step = (1 / clamp(vector_length(delta), 1, 1000)) * 0.1;
	float distance = 0;

	// Get start position
	bezierGetPosition(lastPos, a, b, tA);

	while (tA < tB)
	{
		tA += step;

		// Get new position
		bezierGetPosition(curPos, a, b, tA);

		// compute distance
		vector_subtract(delta, curPos, lastPos);
		vector_copy(lastPos, curPos);
		distance += vector_length(delta);
	}

	return distance;
}

//--------------------------------------------------------------------------
float bezierMove(float* t, BezierPoint_t * a, BezierPoint_t * b, float distance)
{
	VECTOR lastPos, curPos, delta;
	vector_subtract(delta, a->ControlPoint, b->ControlPoint);
	float step = (distance / clamp(vector_length(delta), 1, 1000)) * 0.1;
	float traveled = 0;

	// Get start position
	bezierGetPosition(lastPos, a, b, *t);

	while (*t < 1)
	{
		*t += step;

		// Get new position
		bezierGetPosition(curPos, a, b, *t);

		// Check distance
		vector_subtract(delta, curPos, lastPos);
		vector_copy(lastPos, curPos);
		float len = vector_length(delta);
		if (len < 0.0001)
			break;
		traveled += len;
		if (traveled >= distance)
			break;
	}

	if (*t > 1)
		*t = 1;

	return traveled;
}

//--------------------------------------------------------------------------
float bezierMovePath(float* time, int* index, BezierPoint_t* vertices, int vertexCount, float distance)
{
	float traveled = 0;
	int lastVertex = vertexCount - 1;
	int i = *index;
	int backwards = distance < 0;
	float t = *time;

	if (distance < 0.0001 && distance > -0.0001)
		return 0;
	if (!backwards && i >= lastVertex)
		return 0;
	else if (backwards && i <= 0 && t <= 0)
		return 0;

	if (backwards) {
		t = 1 - t;
		distance = -distance;
	}

	while (distance > 0)
	{
		// move to next vertex
		if (t >= 1)
		{
			if (backwards) {
				if (i <= 0)
					break;
				i--;
				t = 0;
			} else {
				i++;
				t = 0;
				if (i >= lastVertex)
					break;
			}
		}

		float dist = 0;
		if (vertices[i].Disconnected) {
			// if disconnected, jump to the next point
			t = 1;
			dist = 0.0001;
		} else {
			// move along vertex by leftover distance
			if (backwards) {
				dist = bezierMove(&t, &vertices[i+1], &vertices[i], distance);
			} else {
				dist = bezierMove(&t, &vertices[i], &vertices[i+1], distance);
			}
		}

		if (dist < 0.0001)
			break;
		distance -= dist;
		traveled += dist;
	}

	if (backwards) {
		t = 1 - t;
		traveled = -traveled;
	}

	*time = t;
	*index = i;
	return traveled;
}

//--------------------------------------------------------------------------
float bezierGetClosestPointOnPath(VECTOR out, VECTOR position, BezierPoint_t* vertices, float* segmentLengths, int vertexCount)
{
	VECTOR delta, last, temp;
	int i = 0, bestCtrlIdx = 0, lastVertex = vertexCount - 1;
	float t = 0;
	float bestDistSqr = -1;
	float bestTraveled = 0;
	float traveled = 0;

	// find closest control point
	for (i = 0; i < lastVertex; ++i)
	{
		vector_subtract(delta, vertices[i].ControlPoint, position);
		float d = vector_sqrmag(delta);
		if (d < bestDistSqr || bestDistSqr < 0) {
			bestDistSqr = d;
			bestCtrlIdx = i;
		}
	}

	// start on previous point and move two points forward
	if (bestCtrlIdx > 0)
		--bestCtrlIdx;

	// calculate distance up to current point
	i = 0;
	while (i < bestCtrlIdx)
		traveled += segmentLengths[i++];
	
	bestDistSqr = -1;
  while (i < (bestCtrlIdx+2) && i < lastVertex)
	{
		// skip disconnected segments
		if (vertices[i].Disconnected) {
			++i;
			continue;
		}
			
		t = 0;

		// get first position
		bezierGetPosition(last, &vertices[i], &vertices[i+1], t);

		while (1)
		{
			// determine if its the closest one yet
			vector_subtract(delta, last, position);
			float d = vector_sqrmag(delta);
			if (d < bestDistSqr || bestDistSqr < 0) {
				vector_copy(out, temp);
				bestTraveled = traveled;
				bestDistSqr = d;
			}

			// move
			t += 0.01;
			if (t > 1)
				break;
			bezierGetPosition(temp, &vertices[i], &vertices[i+1], t);
			vector_subtract(delta, last, temp);
			vector_copy(last, temp);
			traveled += vector_length(delta);
		}

		++i;
	}

	return bestTraveled;
}
