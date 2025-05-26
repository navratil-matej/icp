#pragma once
#ifndef OBJloader_H
#define OBJloader_H

#include <vector>
#include <glm/fwd.hpp>

#include "Vertex.h"

// struct VertexIdx {
// public:
// 	int PosIdx;
// 	int NormIdx;
// 	int UvIdx;

//     bool operator==(const VertexIdx & rhs)
//     {
//         return this->PosIdx == rhs.PosIdx && this->NormIdx == rhs.NormIdx && this->UvIdx == rhs.UvIdx;
//     }

//     const bool & operator<(const VertexIdx & rhs)
//     {
//         if (PosIdx < rhs.PosIdx) return true;
//         if (NormIdx < rhs.NormIdx) return true;
// 		return UvIdx < rhs.UvIdx;
//     }
// };

bool loadOBJ(
	const char * path,
	std::vector < glm::vec3 > & out_vertices,
	std::vector < glm::vec2 > & out_uvs,
	std::vector < glm::vec3 > & out_normals
);

bool loadOBJidx(
	const char * path,
	std::vector < Vertex > & out_vertices,
	std::vector < GLuint > & out_indices
);

#endif
