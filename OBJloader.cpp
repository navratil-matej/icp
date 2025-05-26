#include <string>
#include <GL/glew.h> 
#include <glm/glm.hpp>
#include <stdio.h>
#include <string.h>
#include <map>

#include <unordered_set>

#include "OBJloader.hpp"
#include "Vertex.h"

#define MAX_LINE_SIZE 255

bool loadOBJ(const char * path, std::vector < glm::vec3 > & out_vertices, std::vector < glm::vec2 > & out_uvs, std::vector < glm::vec3 > & out_normals)
{
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	out_vertices.clear();
	out_uvs.clear();
	out_normals.clear();

	FILE * file;
	file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}

	while (1) {

		char lineHeader[MAX_LINE_SIZE];
		int res = fscanf(file, "%s", lineHeader, MAX_LINE_SIZE);
		if (res == EOF) {
			break;
		}

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.y, &uv.x);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by simple parser :( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
	}

	// unroll from indirect to direct vertex specification
	// sometimes not necessary, definitely not optimal

	for (unsigned int u = 0; u < vertexIndices.size(); u++) {
		unsigned int vertexIndex = vertexIndices[u];
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		out_vertices.push_back(vertex);
	}
	for (unsigned int u = 0; u < uvIndices.size(); u++) {
		unsigned int uvIndex = uvIndices[u];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		out_uvs.push_back(uv);
	}
	for (unsigned int u = 0; u < normalIndices.size(); u++) {
		unsigned int normalIndex = normalIndices[u];
		glm::vec3 normal = temp_normals[normalIndex - 1];
		out_normals.push_back(normal);
	}

	fclose(file);
	return true;
}

bool loadOBJidx(const char * path, std::vector < Vertex > & out_vertices, std::vector < GLuint > & out_indices)
{
	out_vertices.clear();
	out_indices.clear();

	std::map<std::tuple<int, int, int>, int> v2i;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	FILE * file;
	file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}

	while (1) {

		char lineHeader[MAX_LINE_SIZE];
		int res = fscanf(file, "%s", lineHeader, MAX_LINE_SIZE);
		if (res == EOF) {
			break;
		}

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y); // !!!
			uv.y = 1.0f - uv.y; // !!!
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			// VertexIdx vertices[3];
			// int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
			// 	&vertices[0].PosIdx, &vertices[0].UvIdx, &vertices[0].NormIdx,
			// 	&vertices[1].PosIdx, &vertices[1].UvIdx, &vertices[1].NormIdx,
			// 	&vertices[2].PosIdx, &vertices[2].UvIdx, &vertices[2].NormIdx
			// );
			std::tuple<int, int, int> vertices[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
				// Surely this notation breaks the Geneva convention
				&std::get<0>(vertices[0]), &std::get<1>(vertices[0]), &std::get<2>(vertices[0]),
				&std::get<0>(vertices[1]), &std::get<1>(vertices[1]), &std::get<2>(vertices[1]),
				&std::get<0>(vertices[2]), &std::get<1>(vertices[2]), &std::get<2>(vertices[2])
			);
			if (matches != 9) {
				printf("File can't be read by simple parser :( Try exporting with other options\n");
				return false;
			}
			for (auto vi : vertices) {
				auto vp = v2i.find(vi);
				if (vp != v2i.end())
				{
					out_indices.push_back(vp->second);
				}
				else
				{
					unsigned int n = v2i.size();
					v2i.emplace(vi, n);
					out_vertices.push_back(Vertex {
						temp_vertices[std::get<0>(vi) - 1],
						temp_normals [std::get<2>(vi) - 1],
						temp_uvs     [std::get<1>(vi) - 1]
						});
					out_indices.push_back(n);
				}
			}
		}
	}

	fclose(file);
	return true;
}