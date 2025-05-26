#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "ShaderProgram.hpp"
#include "Model.h"

// set uniform according to name 
// https://docs.gl/gl4/glUniform

ShaderProgram::ShaderProgram(const std::filesystem::path& VS_file, const std::filesystem::path& FS_file)
{
	std::vector<GLuint> shader_ids;

	shader_ids.push_back(compile_shader(VS_file, GL_VERTEX_SHADER));
	shader_ids.push_back(compile_shader(FS_file, GL_FRAGMENT_SHADER));

	ID = link_shader(shader_ids);
}

// TODO "inline void" did not function
void ShaderProgram::setUniform(const std::string& name, const float val)
{
	auto loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "no uniform with name:" << name << '\n';
		return;
	}
	glUniform1f(loc, val);
}

void ShaderProgram::setUniform(const std::string& name, const std::vector<float> & vals)
{
	auto loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "no uniform with name:" << name << '\n';
		return;
	}
	glUniform1fv(loc, vals.size(), vals.data());
}

void ShaderProgram::setUniform(const std::string& name, const int val)
{
	auto loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "no uniform with name:" << name << '\n';
		return;
	}
	glUniform1i(loc, val);
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec3 val)
{
	auto loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "no uniform with name:" << name << '\n';
		return;
	}
	glUniform3fv(loc, 1, glm::value_ptr(val));
}

void ShaderProgram::setUniform(const std::string& name, const std::vector<glm::vec3> & vals)
{
	auto loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "no uniform with name:" << name << '\n';
		return;
	}
	glUniform3fv(loc, vals.size(), glm::value_ptr(vals.front()));
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec4 val)
{
	auto loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "no uniform with name:" << name << '\n';
		return;
	}
	glUniform4fv(loc, 1, glm::value_ptr(val));
}

void ShaderProgram::setUniform(const std::string& name, const glm::mat3 val)
{
	auto loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "no uniform with name:" << name << '\n';
		return;
	}
	glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(val));
}

void ShaderProgram::setUniform(const std::string& name, const glm::mat4 val)
{
	auto loc = glGetUniformLocation(ID, name.c_str());
	if (loc == -1)
	{
		std::cerr << "no uniform with name:" << name << '\n';
		return;
	}
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val));
}

std::string ShaderProgram::getShaderInfoLog(const GLuint obj)
{
    int infologLength = 0;
	std::string s;
	glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 0)
	{
		std::vector<char> v(infologLength);
		glGetShaderInfoLog(obj, infologLength, NULL, v.data());
		s.assign(begin(v), end(v));
	}
	return s;
}

std::string ShaderProgram::getProgramInfoLog(const GLuint obj)
{
    int infologLength = 0;
	std::string s;
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 0)
	{
		std::vector<char> v(infologLength);
		glGetShaderInfoLog(obj, infologLength, NULL, v.data());
		s.assign(begin(v), end(v));
	}
	return s;
}

GLuint ShaderProgram::compile_shader(const std::filesystem::path& source_file, const GLenum type)
{
	GLuint shader_h = glCreateShader(type);
	GLint cmpl_status;
	// auto src = textFileRead(source_file).c_str(); // BAD!!!
	auto str = textFileRead(source_file);
	auto src = str.c_str();

	glShaderSource(shader_h, 1, &src, NULL);
	glCompileShader(shader_h);
    glGetShaderiv(shader_h, GL_COMPILE_STATUS, &cmpl_status);

	if (cmpl_status == GL_FALSE)
	{
		std::cerr << getShaderInfoLog(shader_h);
		throw std::runtime_error("Shader compile err.\n");
	}

	return shader_h;
}

GLuint ShaderProgram::link_shader(const std::vector<GLuint> shader_ids)
{
	GLuint prog_h = glCreateProgram();

	for (const auto id : shader_ids)
	{
		glAttachShader(prog_h, id);
	}

	glLinkProgram(prog_h);
	{
		GLint status;
		glGetProgramiv(prog_h, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			std::cerr << getProgramInfoLog(prog_h);
			throw std::runtime_error("Link err.\n");
		}
	}
	return prog_h;
}

std::string ShaderProgram::textFileRead(const std::filesystem::path& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
		throw std::runtime_error("Error opening file.\n");
	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}
