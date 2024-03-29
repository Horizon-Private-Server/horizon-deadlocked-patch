#include <libdl/math3d.h>

const int MOB_PATHFINDING_NODES_COUNT = 18;
const int MOB_PATHFINDING_EDGES_COUNT = 51;

VECTOR MOB_PATHFINDING_NODES[] = {
	{ 195.83, 389.6, 81.01572, 4 },
	{ 192.13, 409.5, 81.01572, 4 },
	{ 214.01, 439.68, 81.63852, 2 },
	{ 196.37, 452.8, 86.0578, 2 },
	{ 171.43, 449.8402, 86.98453, 2 },
	{ 182.4401, 441.1002, 86.77402, 2 },
	{ 164.0701, 436.6402, 84.56702, 2 },
	{ 164.6, 419.1701, 81.01575, 2 },
	{ 191.9301, 429.6001, 81.01575, 4 },
	{ 213.2301, 407.3001, 81.01572, 3 },
	{ 185.39, 376.45, 81.01572, 2 },
	{ 177.2701, 361.9301, 81.00006, 2 },
	{ 193.1501, 366.9301, 82.23447, 2 },
	{ 206.68, 363.05, 81.00009, 1 },
	{ 199.69, 378.54, 81.00012, 1 },
	{ 193.17, 354.5501, 81.00009, 1 },
	{ 183.7, 452.0901, 86.85968, 2 },
	{ 212.9501, 422.6101, 86.97128, 2 },
};

u8 MOB_PATHFINDING_NODES_CORNERING[] = {
	191,
	191,
	0,
	0,
	0,
	0,
	0,
	0,
	191,
	191,
	191,
	191,
	191,
	0,
	0,
	191,
	0,
	0,
};

u8 MOB_PATHFINDING_EDGES[][2] = {
	{ 0, 9 },
	{ 9, 0 },
	{ 0, 14 },
	{ 14, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 0, 10 },
	{ 10, 0 },
	{ 1, 9 },
	{ 9, 1 },
	{ 1, 8 },
	{ 8, 1 },
	{ 1, 7 },
	{ 7, 1 },
	{ 8, 2 },
	{ 2, 8 },
	{ 2, 3 },
	{ 3, 2 },
	{ 3, 5 },
	{ 5, 3 },
	{ 5, 6 },
	{ 6, 5 },
	{ 6, 4 },
	{ 4, 6 },
	{ 6, 7 },
	{ 7, 6 },
	{ 5, 8 },
	{ 10, 11 },
	{ 11, 10 },
	{ 11, 12 },
	{ 12, 11 },
	{ 11, 15 },
	{ 15, 11 },
	{ 12, 13 },
	{ 13, 12 },
	{ 13, 14 },
	{ 14, 13 },
	{ 13, 15 },
	{ 15, 13 },
	{ 16, 3 },
	{ 3, 16 },
	{ 16, 4 },
	{ 4, 16 },
	{ 16, 5 },
	{ 5, 16 },
	{ 17, 9 },
	{ 9, 17 },
	{ 17, 8 },
	{ 8, 17 },
	{ 17, 1 },
	{ 1, 17 },
};

u8 MOB_PATHFINDING_EDGES_REQUIRED[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	255,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	255,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

u8 MOB_PATHFINDING_EDGES_PATHFIT[] = {
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
};

u8 MOB_PATHFINDING_EDGES_JUMPPADSPEED[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

u8 MOB_PATHFINDING_EDGES_JUMPPADAT[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

const int MOB_PATHFINDING_PATHS_MAX_PATH_LENGTH = 8;
u8 MOB_PATHFINDING_PATHS[][8] = {
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 4, 255, 255, 255, 255, 255, 255, 255, },
	{ 4, 10, 14, 255, 255, 255, 255, 255, },
	{ 4, 10, 14, 16, 255, 255, 255, 255, },
	{ 4, 12, 25, 22, 255, 255, 255, 255, },
	{ 4, 12, 25, 21, 255, 255, 255, 255, },
	{ 4, 12, 25, 255, 255, 255, 255, 255, },
	{ 4, 12, 255, 255, 255, 255, 255, 255, },
	{ 4, 10, 255, 255, 255, 255, 255, 255, },
	{ 0, 255, 255, 255, 255, 255, 255, 255, },
	{ 6, 255, 255, 255, 255, 255, 255, 255, },
	{ 6, 27, 255, 255, 255, 255, 255, 255, },
	{ 2, 36, 34, 255, 255, 255, 255, 255, },
	{ 2, 36, 255, 255, 255, 255, 255, 255, },
	{ 2, 255, 255, 255, 255, 255, 255, 255, },
	{ 2, 36, 37, 255, 255, 255, 255, 255, },
	{ 4, 12, 25, 22, 42, 255, 255, 255, },
	{ 0, 46, 255, 255, 255, 255, 255, 255, },
	{ 5, 255, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 10, 14, 255, 255, 255, 255, 255, 255, },
	{ 10, 14, 16, 255, 255, 255, 255, 255, },
	{ 12, 25, 22, 255, 255, 255, 255, 255, },
	{ 12, 25, 21, 255, 255, 255, 255, 255, },
	{ 12, 25, 255, 255, 255, 255, 255, 255, },
	{ 12, 255, 255, 255, 255, 255, 255, 255, },
	{ 10, 255, 255, 255, 255, 255, 255, 255, },
	{ 8, 255, 255, 255, 255, 255, 255, 255, },
	{ 5, 6, 255, 255, 255, 255, 255, 255, },
	{ 5, 6, 27, 255, 255, 255, 255, 255, },
	{ 5, 2, 36, 34, 255, 255, 255, 255, },
	{ 5, 2, 36, 255, 255, 255, 255, 255, },
	{ 5, 2, 255, 255, 255, 255, 255, 255, },
	{ 5, 2, 36, 37, 255, 255, 255, 255, },
	{ 12, 25, 22, 42, 255, 255, 255, 255, },
	{ 50, 255, 255, 255, 255, 255, 255, 255, },
	{ 15, 11, 5, 255, 255, 255, 255, 255, },
	{ 15, 11, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 16, 255, 255, 255, 255, 255, 255, 255, },
	{ 16, 40, 41, 255, 255, 255, 255, 255, },
	{ 16, 18, 255, 255, 255, 255, 255, 255, },
	{ 16, 18, 20, 255, 255, 255, 255, 255, },
	{ 15, 11, 12, 255, 255, 255, 255, 255, },
	{ 15, 255, 255, 255, 255, 255, 255, 255, },
	{ 15, 48, 45, 255, 255, 255, 255, 255, },
	{ 15, 11, 5, 6, 255, 255, 255, 255, },
	{ 15, 11, 5, 6, 27, 255, 255, 255, },
	{ 15, 11, 5, 2, 36, 34, 255, 255, },
	{ 15, 11, 5, 2, 36, 255, 255, 255, },
	{ 15, 11, 5, 2, 255, 255, 255, 255, },
	{ 15, 11, 5, 2, 36, 37, 255, 255, },
	{ 16, 40, 255, 255, 255, 255, 255, 255, },
	{ 15, 48, 255, 255, 255, 255, 255, 255, },
	{ 18, 26, 11, 5, 255, 255, 255, 255, },
	{ 18, 26, 11, 255, 255, 255, 255, 255, },
	{ 17, 255, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 40, 41, 255, 255, 255, 255, 255, 255, },
	{ 18, 255, 255, 255, 255, 255, 255, 255, },
	{ 18, 20, 255, 255, 255, 255, 255, 255, },
	{ 18, 20, 24, 255, 255, 255, 255, 255, },
	{ 18, 26, 255, 255, 255, 255, 255, 255, },
	{ 18, 26, 48, 45, 255, 255, 255, 255, },
	{ 18, 26, 11, 5, 6, 255, 255, 255, },
	{ 18, 26, 11, 5, 6, 27, 255, 255, },
	{ 18, 26, 11, 5, 2, 36, 34, 255, },
	{ 18, 26, 11, 5, 2, 36, 255, 255, },
	{ 18, 26, 11, 5, 2, 255, 255, 255, },
	{ 18, 26, 11, 5, 2, 36, 37, 255, },
	{ 40, 255, 255, 255, 255, 255, 255, 255, },
	{ 18, 26, 48, 255, 255, 255, 255, 255, },
	{ 42, 43, 26, 11, 5, 255, 255, 255, },
	{ 42, 43, 26, 11, 255, 255, 255, 255, },
	{ 42, 39, 17, 255, 255, 255, 255, 255, },
	{ 42, 39, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 42, 43, 255, 255, 255, 255, 255, 255, },
	{ 23, 255, 255, 255, 255, 255, 255, 255, },
	{ 23, 24, 255, 255, 255, 255, 255, 255, },
	{ 42, 43, 26, 255, 255, 255, 255, 255, },
	{ 42, 43, 26, 48, 45, 255, 255, 255, },
	{ 42, 43, 26, 11, 5, 6, 255, 255, },
	{ 42, 43, 26, 11, 5, 6, 27, 255, },
	{ 42, 43, 26, 11, 5, 2, 36, 34, },
	{ 42, 43, 26, 11, 5, 2, 36, 255, },
	{ 42, 43, 26, 11, 5, 2, 255, 255, },
	{ 42, 43, 26, 11, 5, 2, 36, 37, },
	{ 42, 255, 255, 255, 255, 255, 255, 255, },
	{ 42, 43, 26, 48, 255, 255, 255, 255, },
	{ 26, 11, 5, 255, 255, 255, 255, 255, },
	{ 26, 11, 255, 255, 255, 255, 255, 255, },
	{ 26, 14, 255, 255, 255, 255, 255, 255, },
	{ 19, 255, 255, 255, 255, 255, 255, 255, },
	{ 44, 41, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 20, 255, 255, 255, 255, 255, 255, 255, },
	{ 20, 24, 255, 255, 255, 255, 255, 255, },
	{ 26, 255, 255, 255, 255, 255, 255, 255, },
	{ 26, 48, 45, 255, 255, 255, 255, 255, },
	{ 26, 11, 5, 6, 255, 255, 255, 255, },
	{ 26, 11, 5, 6, 27, 255, 255, 255, },
	{ 26, 11, 5, 2, 36, 34, 255, 255, },
	{ 26, 11, 5, 2, 36, 255, 255, 255, },
	{ 26, 11, 5, 2, 255, 255, 255, 255, },
	{ 26, 11, 5, 2, 36, 37, 255, 255, },
	{ 44, 255, 255, 255, 255, 255, 255, 255, },
	{ 26, 48, 255, 255, 255, 255, 255, 255, },
	{ 24, 13, 5, 255, 255, 255, 255, 255, },
	{ 24, 13, 255, 255, 255, 255, 255, 255, },
	{ 21, 26, 14, 255, 255, 255, 255, 255, },
	{ 21, 19, 255, 255, 255, 255, 255, 255, },
	{ 22, 255, 255, 255, 255, 255, 255, 255, },
	{ 21, 255, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 24, 255, 255, 255, 255, 255, 255, 255, },
	{ 21, 26, 255, 255, 255, 255, 255, 255, },
	{ 24, 13, 8, 255, 255, 255, 255, 255, },
	{ 24, 13, 5, 6, 255, 255, 255, 255, },
	{ 24, 13, 5, 6, 27, 255, 255, 255, },
	{ 24, 13, 5, 2, 36, 34, 255, 255, },
	{ 24, 13, 5, 2, 36, 255, 255, 255, },
	{ 24, 13, 5, 2, 255, 255, 255, 255, },
	{ 24, 13, 5, 2, 36, 37, 255, 255, },
	{ 22, 42, 255, 255, 255, 255, 255, 255, },
	{ 21, 26, 48, 255, 255, 255, 255, 255, },
	{ 13, 5, 255, 255, 255, 255, 255, 255, },
	{ 13, 255, 255, 255, 255, 255, 255, 255, },
	{ 13, 10, 14, 255, 255, 255, 255, 255, },
	{ 25, 21, 19, 255, 255, 255, 255, 255, },
	{ 25, 22, 255, 255, 255, 255, 255, 255, },
	{ 25, 21, 255, 255, 255, 255, 255, 255, },
	{ 25, 255, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 13, 10, 255, 255, 255, 255, 255, 255, },
	{ 13, 8, 255, 255, 255, 255, 255, 255, },
	{ 13, 5, 6, 255, 255, 255, 255, 255, },
	{ 13, 5, 6, 27, 255, 255, 255, 255, },
	{ 13, 5, 2, 36, 34, 255, 255, 255, },
	{ 13, 5, 2, 36, 255, 255, 255, 255, },
	{ 13, 5, 2, 255, 255, 255, 255, 255, },
	{ 13, 5, 2, 36, 37, 255, 255, 255, },
	{ 25, 22, 42, 255, 255, 255, 255, 255, },
	{ 13, 50, 255, 255, 255, 255, 255, 255, },
	{ 11, 5, 255, 255, 255, 255, 255, 255, },
	{ 11, 255, 255, 255, 255, 255, 255, 255, },
	{ 14, 255, 255, 255, 255, 255, 255, 255, },
	{ 14, 16, 255, 255, 255, 255, 255, 255, },
	{ 14, 16, 40, 41, 255, 255, 255, 255, },
	{ 14, 16, 18, 255, 255, 255, 255, 255, },
	{ 11, 12, 25, 255, 255, 255, 255, 255, },
	{ 11, 12, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 48, 45, 255, 255, 255, 255, 255, 255, },
	{ 11, 5, 6, 255, 255, 255, 255, 255, },
	{ 11, 5, 6, 27, 255, 255, 255, 255, },
	{ 11, 5, 2, 36, 34, 255, 255, 255, },
	{ 11, 5, 2, 36, 255, 255, 255, 255, },
	{ 11, 5, 2, 255, 255, 255, 255, 255, },
	{ 11, 5, 2, 36, 37, 255, 255, 255, },
	{ 14, 16, 40, 255, 255, 255, 255, 255, },
	{ 48, 255, 255, 255, 255, 255, 255, 255, },
	{ 1, 255, 255, 255, 255, 255, 255, 255, },
	{ 9, 255, 255, 255, 255, 255, 255, 255, },
	{ 46, 47, 14, 255, 255, 255, 255, 255, },
	{ 46, 47, 14, 16, 255, 255, 255, 255, },
	{ 9, 12, 25, 22, 255, 255, 255, 255, },
	{ 9, 12, 25, 21, 255, 255, 255, 255, },
	{ 9, 12, 25, 255, 255, 255, 255, 255, },
	{ 9, 12, 255, 255, 255, 255, 255, 255, },
	{ 46, 47, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 1, 6, 255, 255, 255, 255, 255, 255, },
	{ 1, 6, 27, 255, 255, 255, 255, 255, },
	{ 1, 2, 36, 34, 255, 255, 255, 255, },
	{ 1, 2, 36, 255, 255, 255, 255, 255, },
	{ 1, 2, 255, 255, 255, 255, 255, 255, },
	{ 1, 2, 36, 37, 255, 255, 255, 255, },
	{ 9, 12, 25, 22, 42, 255, 255, 255, },
	{ 46, 255, 255, 255, 255, 255, 255, 255, },
	{ 7, 255, 255, 255, 255, 255, 255, 255, },
	{ 7, 4, 255, 255, 255, 255, 255, 255, },
	{ 7, 4, 10, 14, 255, 255, 255, 255, },
	{ 7, 4, 10, 14, 16, 255, 255, 255, },
	{ 7, 4, 12, 25, 22, 255, 255, 255, },
	{ 7, 4, 12, 25, 21, 255, 255, 255, },
	{ 7, 4, 12, 25, 255, 255, 255, 255, },
	{ 7, 4, 12, 255, 255, 255, 255, 255, },
	{ 7, 4, 10, 255, 255, 255, 255, 255, },
	{ 7, 0, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 27, 255, 255, 255, 255, 255, 255, 255, },
	{ 27, 29, 255, 255, 255, 255, 255, 255, },
	{ 7, 2, 36, 255, 255, 255, 255, 255, },
	{ 7, 2, 255, 255, 255, 255, 255, 255, },
	{ 27, 31, 255, 255, 255, 255, 255, 255, },
	{ 7, 4, 12, 25, 22, 42, 255, 255, },
	{ 7, 0, 46, 255, 255, 255, 255, 255, },
	{ 28, 7, 255, 255, 255, 255, 255, 255, },
	{ 28, 7, 4, 255, 255, 255, 255, 255, },
	{ 28, 7, 4, 10, 14, 255, 255, 255, },
	{ 28, 7, 4, 10, 14, 16, 255, 255, },
	{ 28, 7, 4, 12, 25, 22, 255, 255, },
	{ 28, 7, 4, 12, 25, 21, 255, 255, },
	{ 28, 7, 4, 12, 25, 255, 255, 255, },
	{ 28, 7, 4, 12, 255, 255, 255, 255, },
	{ 28, 7, 4, 10, 255, 255, 255, 255, },
	{ 28, 7, 0, 255, 255, 255, 255, 255, },
	{ 28, 255, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 29, 255, 255, 255, 255, 255, 255, 255, },
	{ 29, 33, 255, 255, 255, 255, 255, 255, },
	{ 28, 7, 2, 255, 255, 255, 255, 255, },
	{ 31, 255, 255, 255, 255, 255, 255, 255, },
	{ 28, 7, 4, 12, 25, 22, 42, 255, },
	{ 28, 7, 0, 46, 255, 255, 255, 255, },
	{ 33, 35, 3, 255, 255, 255, 255, 255, },
	{ 33, 35, 3, 4, 255, 255, 255, 255, },
	{ 33, 35, 3, 4, 10, 14, 255, 255, },
	{ 33, 35, 3, 4, 10, 14, 16, 255, },
	{ 33, 35, 3, 4, 12, 25, 22, 255, },
	{ 33, 35, 3, 4, 12, 25, 21, 255, },
	{ 33, 35, 3, 4, 12, 25, 255, 255, },
	{ 33, 35, 3, 4, 12, 255, 255, 255, },
	{ 33, 35, 3, 4, 10, 255, 255, 255, },
	{ 33, 35, 3, 0, 255, 255, 255, 255, },
	{ 30, 28, 255, 255, 255, 255, 255, 255, },
	{ 30, 255, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 33, 255, 255, 255, 255, 255, 255, 255, },
	{ 33, 35, 255, 255, 255, 255, 255, 255, },
	{ 33, 37, 255, 255, 255, 255, 255, 255, },
	{ 33, 35, 3, 4, 12, 25, 22, 42, },
	{ 33, 35, 3, 0, 46, 255, 255, 255, },
	{ 35, 3, 255, 255, 255, 255, 255, 255, },
	{ 35, 3, 4, 255, 255, 255, 255, 255, },
	{ 35, 3, 4, 10, 14, 255, 255, 255, },
	{ 35, 3, 4, 10, 14, 16, 255, 255, },
	{ 35, 3, 4, 12, 25, 22, 255, 255, },
	{ 35, 3, 4, 12, 25, 21, 255, 255, },
	{ 35, 3, 4, 12, 25, 255, 255, 255, },
	{ 35, 3, 4, 12, 255, 255, 255, 255, },
	{ 35, 3, 4, 10, 255, 255, 255, 255, },
	{ 35, 3, 0, 255, 255, 255, 255, 255, },
	{ 35, 3, 6, 255, 255, 255, 255, 255, },
	{ 34, 30, 255, 255, 255, 255, 255, 255, },
	{ 34, 255, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 35, 255, 255, 255, 255, 255, 255, 255, },
	{ 37, 255, 255, 255, 255, 255, 255, 255, },
	{ 35, 3, 4, 12, 25, 22, 42, 255, },
	{ 35, 3, 0, 46, 255, 255, 255, 255, },
	{ 3, 255, 255, 255, 255, 255, 255, 255, },
	{ 3, 4, 255, 255, 255, 255, 255, 255, },
	{ 3, 4, 10, 14, 255, 255, 255, 255, },
	{ 3, 4, 10, 14, 16, 255, 255, 255, },
	{ 3, 4, 12, 25, 22, 255, 255, 255, },
	{ 3, 4, 12, 25, 21, 255, 255, 255, },
	{ 3, 4, 12, 25, 255, 255, 255, 255, },
	{ 3, 4, 12, 255, 255, 255, 255, 255, },
	{ 3, 4, 10, 255, 255, 255, 255, 255, },
	{ 3, 0, 255, 255, 255, 255, 255, 255, },
	{ 3, 6, 255, 255, 255, 255, 255, 255, },
	{ 3, 6, 27, 255, 255, 255, 255, 255, },
	{ 36, 34, 255, 255, 255, 255, 255, 255, },
	{ 36, 255, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 36, 37, 255, 255, 255, 255, 255, 255, },
	{ 3, 4, 12, 25, 22, 42, 255, 255, },
	{ 3, 0, 46, 255, 255, 255, 255, 255, },
	{ 38, 35, 3, 255, 255, 255, 255, 255, },
	{ 38, 35, 3, 4, 255, 255, 255, 255, },
	{ 38, 35, 3, 4, 10, 14, 255, 255, },
	{ 38, 35, 3, 4, 10, 14, 16, 255, },
	{ 38, 35, 3, 4, 12, 25, 22, 255, },
	{ 38, 35, 3, 4, 12, 25, 21, 255, },
	{ 38, 35, 3, 4, 12, 25, 255, 255, },
	{ 38, 35, 3, 4, 12, 255, 255, 255, },
	{ 38, 35, 3, 4, 10, 255, 255, 255, },
	{ 38, 35, 3, 0, 255, 255, 255, 255, },
	{ 32, 28, 255, 255, 255, 255, 255, 255, },
	{ 32, 255, 255, 255, 255, 255, 255, 255, },
	{ 38, 34, 255, 255, 255, 255, 255, 255, },
	{ 38, 255, 255, 255, 255, 255, 255, 255, },
	{ 38, 35, 255, 255, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 38, 35, 3, 4, 12, 25, 22, 42, },
	{ 38, 35, 3, 0, 46, 255, 255, 255, },
	{ 43, 26, 11, 5, 255, 255, 255, 255, },
	{ 43, 26, 11, 255, 255, 255, 255, 255, },
	{ 39, 17, 255, 255, 255, 255, 255, 255, },
	{ 39, 255, 255, 255, 255, 255, 255, 255, },
	{ 41, 255, 255, 255, 255, 255, 255, 255, },
	{ 43, 255, 255, 255, 255, 255, 255, 255, },
	{ 41, 23, 255, 255, 255, 255, 255, 255, },
	{ 41, 23, 24, 255, 255, 255, 255, 255, },
	{ 43, 26, 255, 255, 255, 255, 255, 255, },
	{ 43, 26, 48, 45, 255, 255, 255, 255, },
	{ 43, 26, 11, 5, 6, 255, 255, 255, },
	{ 43, 26, 11, 5, 6, 27, 255, 255, },
	{ 43, 26, 11, 5, 2, 36, 34, 255, },
	{ 43, 26, 11, 5, 2, 36, 255, 255, },
	{ 43, 26, 11, 5, 2, 255, 255, 255, },
	{ 43, 26, 11, 5, 2, 36, 37, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
	{ 43, 26, 48, 255, 255, 255, 255, 255, },
	{ 45, 1, 255, 255, 255, 255, 255, 255, },
	{ 49, 255, 255, 255, 255, 255, 255, 255, },
	{ 47, 14, 255, 255, 255, 255, 255, 255, },
	{ 47, 14, 16, 255, 255, 255, 255, 255, },
	{ 49, 12, 25, 22, 255, 255, 255, 255, },
	{ 47, 14, 16, 18, 255, 255, 255, 255, },
	{ 49, 12, 25, 255, 255, 255, 255, 255, },
	{ 49, 12, 255, 255, 255, 255, 255, 255, },
	{ 47, 255, 255, 255, 255, 255, 255, 255, },
	{ 45, 255, 255, 255, 255, 255, 255, 255, },
	{ 45, 1, 6, 255, 255, 255, 255, 255, },
	{ 45, 1, 6, 27, 255, 255, 255, 255, },
	{ 45, 1, 2, 36, 34, 255, 255, 255, },
	{ 45, 1, 2, 36, 255, 255, 255, 255, },
	{ 45, 1, 2, 255, 255, 255, 255, 255, },
	{ 45, 1, 2, 36, 37, 255, 255, 255, },
	{ 47, 14, 16, 40, 255, 255, 255, 255, },
	{ 255, 255, 255, 255, 255, 255, 255, 255, },
};

