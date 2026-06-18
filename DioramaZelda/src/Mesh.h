/* Mesh.h - Geometria: carregamento de OBJ e geometria procedural
 *
 * Diorama Zelda - Visualizador Final 3D - Computação Gráfica / Unisinos
 *
 * Formato de vértice UNIFICADO (8 floats):
 *     posição (x,y,z)  |  normal (nx,ny,nz)  |  uv (u,v)
 *     atributo 0          atributo 1            atributo 2
 *
 * Esse formato serve tanto para o mapeamento de textura quanto para o
 * cálculo de iluminação (que precisa das normais).
 *
 * Reaproveita o loadSimpleOBJ / loadTexture dos trabalhos anteriores,
 * estendidos para empacotar também a normal.
 */
#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <stb_image.h>

struct Mesh
{
    GLuint vao       = 0;
    int    nVertices = 0;
};

// ─────────────────────────────────────────────────────────────
// Helpers de construção
// ─────────────────────────────────────────────────────────────

// Empacota um vértice (pos + normal + uv) no buffer.
inline void pushVertex(std::vector<float>& buf,
                       const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv)
{
    buf.push_back(p.x); buf.push_back(p.y); buf.push_back(p.z);
    buf.push_back(n.x); buf.push_back(n.y); buf.push_back(n.z);
    buf.push_back(uv.x); buf.push_back(uv.y);
}

// Cria o VAO/VBO a partir de um buffer intercalado de 8 floats por vértice.
inline Mesh createMesh(const std::vector<float>& buf)
{
    Mesh mesh;
    mesh.nVertices = (int)(buf.size() / 8);

    GLuint VBO;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(float), buf.data(), GL_STATIC_DRAW);

    const GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return mesh;
}

// ─────────────────────────────────────────────────────────────
// Carregamento de OBJ (com normais)
// ─────────────────────────────────────────────────────────────

inline Mesh loadSimpleOBJ(const std::string& filePath)
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<float>     vBuffer;

    std::ifstream arq(filePath.c_str());
    if (!arq.is_open())
    {
        std::cerr << "[OBJ] Erro ao abrir: " << filePath << std::endl;
        return Mesh{};
    }

    std::string line;
    while (std::getline(arq, line))
    {
        std::istringstream ss(line);
        std::string word;
        ss >> word;

        if (word == "v")
        {
            glm::vec3 v; ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (word == "vt")
        {
            glm::vec2 vt; ss >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 vn; ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (word == "f")
        {
            // Coleta todos os índices da face (suporta triângulos e quads).
            std::vector<int> vi, ti, ni;
            std::string token;
            while (ss >> token)
            {
                int iv = 0, it = 0, in = 0;
                std::istringstream fs(token);
                std::string idx;
                if (std::getline(fs, idx, '/')) iv = !idx.empty() ? std::stoi(idx) - 1 : -1;
                if (std::getline(fs, idx, '/')) it = !idx.empty() ? std::stoi(idx) - 1 : -1;
                if (std::getline(fs, idx))      in = !idx.empty() ? std::stoi(idx) - 1 : -1;
                vi.push_back(iv); ti.push_back(it); ni.push_back(in);
            }

            // Triangulação em leque: (0, k, k+1)
            for (size_t k = 1; k + 1 < vi.size(); ++k)
            {
                int tri[3] = { (int)0, (int)k, (int)(k + 1) };

                // Normal geométrica caso o OBJ não traga vn.
                glm::vec3 faceN(0.0f);
                if (ni[tri[0]] < 0 || normals.empty())
                {
                    glm::vec3 a = positions[vi[tri[0]]];
                    glm::vec3 b = positions[vi[tri[1]]];
                    glm::vec3 c = positions[vi[tri[2]]];
                    faceN = glm::normalize(glm::cross(b - a, c - a));
                }

                for (int j = 0; j < 3; ++j)
                {
                    int t = tri[j];
                    glm::vec3 p = positions[vi[t]];
                    glm::vec3 n = (ni[t] >= 0 && !normals.empty()) ? normals[ni[t]] : faceN;
                    glm::vec2 uv = (ti[t] >= 0 && !texCoords.empty()) ? texCoords[ti[t]] : glm::vec2(0.0f);
                    pushVertex(vBuffer, p, n, uv);
                }
            }
        }
    }
    arq.close();

    Mesh mesh = createMesh(vBuffer);
    std::cout << "[OBJ] " << filePath << " (" << mesh.nVertices << " vertices)" << std::endl;
    return mesh;
}

// ─────────────────────────────────────────────────────────────
// Textura
// ─────────────────────────────────────────────────────────────

inline GLuint loadTexture(const std::string& filePath)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int w, h, ch;
    unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &ch, 0);
    if (data)
    {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "[TEX] " << filePath << " (" << w << "x" << h << ")" << std::endl;
    }
    else
    {
        std::cout << "[TEX] Falha ao carregar: " << filePath << std::endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

// ─────────────────────────────────────────────────────────────
// Geometria procedural (tema Zelda)
// ─────────────────────────────────────────────────────────────

// Chão de Hyrule: um quad grande no plano y=0, normal para cima, uv repetido.
inline Mesh makeGround(float size = 20.0f, float tile = 8.0f)
{
    glm::vec3 n(0.0f, 1.0f, 0.0f);
    std::vector<float> buf;
    glm::vec3 a(-size, 0.0f, -size), b(size, 0.0f, -size),
              c(size, 0.0f,  size), d(-size, 0.0f,  size);
    pushVertex(buf, a, n, glm::vec2(0, 0));
    pushVertex(buf, b, n, glm::vec2(tile, 0));
    pushVertex(buf, c, n, glm::vec2(tile, tile));
    pushVertex(buf, a, n, glm::vec2(0, 0));
    pushVertex(buf, c, n, glm::vec2(tile, tile));
    pushVertex(buf, d, n, glm::vec2(0, tile));
    return createMesh(buf);
}

// Triforce: 3 triângulos equiláteros coplanares (o do meio fica "vazio"),
// formando o emblema clássico. Fica no plano XY, normal +Z.
inline Mesh makeTriforce()
{
    std::vector<float> buf;
    glm::vec3 n(0.0f, 0.0f, 1.0f);
    const float h = 0.8660254f; // altura de um triângulo equilátero de lado 1

    auto addTri = [&](glm::vec3 off)
    {
        glm::vec3 p0 = off + glm::vec3(-0.5f, 0.0f, 0.0f);
        glm::vec3 p1 = off + glm::vec3( 0.5f, 0.0f, 0.0f);
        glm::vec3 p2 = off + glm::vec3( 0.0f, h,    0.0f);
        pushVertex(buf, p0, n, glm::vec2(0, 0));
        pushVertex(buf, p1, n, glm::vec2(1, 0));
        pushVertex(buf, p2, n, glm::vec2(0.5f, 1));
    };

    addTri(glm::vec3(-0.5f, 0.0f,   0.0f)); // inferior esquerdo
    addTri(glm::vec3( 0.5f, 0.0f,   0.0f)); // inferior direito
    addTri(glm::vec3( 0.0f, h,      0.0f)); // superior
    return createMesh(buf);
}

// Rupia: octaedro (diamante). Centro na origem.
inline Mesh makeRupee(float r = 0.5f)
{
    std::vector<float> buf;
    glm::vec3 top(0, r, 0), bot(0, -r, 0);
    glm::vec3 e[4] = {
        glm::vec3( r, 0,  0), glm::vec3( 0, 0,  r),
        glm::vec3(-r, 0,  0), glm::vec3( 0, 0, -r)
    };
    auto addFace = [&](glm::vec3 A, glm::vec3 B, glm::vec3 C)
    {
        glm::vec3 nn = glm::normalize(glm::cross(B - A, C - A));
        pushVertex(buf, A, nn, glm::vec2(0.5f, 1));
        pushVertex(buf, B, nn, glm::vec2(0, 0));
        pushVertex(buf, C, nn, glm::vec2(1, 0));
    };
    for (int i = 0; i < 4; ++i)
    {
        glm::vec3 a = e[i], b = e[(i + 1) % 4];
        addFace(top, a, b);     // metade superior
        addFace(bot, b, a);     // metade inferior
    }
    return createMesh(buf);
}

// Cubo procedural com normais e uv por face (usado como baú).
inline Mesh makeCube(float s = 0.5f)
{
    std::vector<float> buf;
    struct Face { glm::vec3 n; glm::vec3 v[4]; };
    glm::vec3 P[8] = {
        {-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s},
        {-s,-s,-s},{ s,-s,-s},{ s, s,-s},{-s, s,-s}
    };
    Face faces[6] = {
        {{ 0, 0, 1},{P[0],P[1],P[2],P[3]}}, // frente
        {{ 0, 0,-1},{P[5],P[4],P[7],P[6]}}, // trás
        {{-1, 0, 0},{P[4],P[0],P[3],P[7]}}, // esquerda
        {{ 1, 0, 0},{P[1],P[5],P[6],P[2]}}, // direita
        {{ 0, 1, 0},{P[3],P[2],P[6],P[7]}}, // topo
        {{ 0,-1, 0},{P[4],P[5],P[1],P[0]}}  // base
    };
    for (auto& f : faces)
    {
        pushVertex(buf, f.v[0], f.n, glm::vec2(0, 0));
        pushVertex(buf, f.v[1], f.n, glm::vec2(1, 0));
        pushVertex(buf, f.v[2], f.n, glm::vec2(1, 1));
        pushVertex(buf, f.v[0], f.n, glm::vec2(0, 0));
        pushVertex(buf, f.v[2], f.n, glm::vec2(1, 1));
        pushVertex(buf, f.v[3], f.n, glm::vec2(0, 1));
    }
    return createMesh(buf);
}

// Esfera UV (base: inicializaEsfera de CuboIluminacao.cpp), agora com uv.
inline Mesh makeSphere(float radius = 0.5f, int stacks = 24, int sectors = 24)
{
    std::vector<float> buf;
    const float PI = 3.14159265359f;
    auto vtx = [&](float phi, float theta)
    {
        float x = radius * sin(phi) * cos(theta);
        float y = radius * cos(phi);
        float z = radius * sin(phi) * sin(theta);
        glm::vec3 p(x, y, z);
        glm::vec3 n = glm::normalize(p);
        glm::vec2 uv(theta / (2 * PI), 1.0f - phi / PI);
        pushVertex(buf, p, n, uv);
    };
    for (int i = 0; i < stacks; ++i)
    {
        float phi1 = PI * float(i) / stacks;
        float phi2 = PI * float(i + 1) / stacks;
        for (int j = 0; j < sectors; ++j)
        {
            float t1 = 2.0f * PI * float(j) / sectors;
            float t2 = 2.0f * PI * float(j + 1) / sectors;
            vtx(phi1, t1); vtx(phi2, t1); vtx(phi1, t2);
            vtx(phi1, t2); vtx(phi2, t1); vtx(phi2, t2);
        }
    }
    return createMesh(buf);
}

// Pequena esfera/cubo para marcar visualmente uma fonte de luz.
inline Mesh makeLightMarker() { return makeSphere(0.15f, 10, 10); }

// Despacha a geometria a partir de uma "spec":
//   "PROC:triforce" | "PROC:rupee" | "PROC:ground" | "PROC:cube" | "PROC:sphere"
//   ou um caminho para arquivo .obj.
inline Mesh loadGeometry(const std::string& spec)
{
    if (spec.rfind("PROC:", 0) == 0)
    {
        std::string kind = spec.substr(5);
        if (kind == "triforce") return makeTriforce();
        if (kind == "rupee")    return makeRupee();
        if (kind == "ground")   return makeGround();
        if (kind == "cube")     return makeCube();
        if (kind == "sphere")   return makeSphere();
        std::cerr << "[GEO] Procedural desconhecida: " << kind << std::endl;
        return makeCube();
    }
    return loadSimpleOBJ(spec);
}
