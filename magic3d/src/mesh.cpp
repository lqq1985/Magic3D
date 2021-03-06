/******************************************************************************
 @Copyright    Copyright (C) 2008 - 2016 by MagicTech.

 @Platform     ANSI compatible
******************************************************************************/
/*
Magic3D Engine
Copyright (c) 2008-2016
Thiago C. Moraes
http://www.magictech.com.br

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
   If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <magic3d/resource/resourcemngr.h>
#include <magic3d/renderer/renderer.h>

Magic3D::MeshData::MeshData(const MeshData& meshData)
{
    this->vertices  = meshData.vertices;
    this->indexes   = meshData.indexes;
    this->lines     = meshData.lines;
    this->points    = meshData.points;

    this->name = meshData.name;

    this->shape = NULL;
    this->triangleMesh = NULL;

    this->skinned = meshData.skinned;

    this->box = meshData.box;
    this->computedBox = meshData.computedBox;

    this->type = meshData.type;

    renderID.id = 0;
    renderID.data  = 0;
    renderID.dataSize = 0;
    renderID.index = 0;
    renderID.indexSize = 0;

    if (meshData.renderID.data > 0 && meshData.renderID.index > 0)
    {
        Renderer* renderer = Renderer::getInstance();
        if (renderer)
        {
            if (!renderer->hasMapBuffer())
            {
                createVbo();
            }
            else
            {
                this->renderID = renderer->copyVBO(meshData.renderID);
            }
        }
    }
}

Magic3D::MeshData::MeshData(MESH type, std::string name)
{
    this->type = type;
    this->name = name;

    this->shape = NULL;
    this->triangleMesh = NULL;

    skinned = false;

    box = Box(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));
    computedBox = false;

    renderID.id = 0;
    renderID.data  = 0;
    renderID.dataSize = 0;
    renderID.index = 0;
    renderID.indexSize = 0;
}

Magic3D::MeshData::~MeshData()
{
    deleteVbo();

    if (this->shape)
    {
        delete this->shape;
        this->shape = NULL;
    }

    if (this->triangleMesh)
    {
        delete this->triangleMesh;
        this->triangleMesh = NULL;
    }
}

void* Magic3D::MeshData::spawn() const
{
    return (new MeshData(*this));
}

bool Magic3D::MeshData::update(Object* object)
{
#if defined(MAGIC3D_LEGACY)
    bool first = true;
    Vector3 min = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 max = Vector3(0.0f, 0.0f, 0.0f);

    std::vector<Vertex3D>::iterator it_s = vertices.begin();

    int inc = sizeof(Vertex3D) / sizeof(float);
    int incp = sizeof(btVector3) / sizeof(float);
    int vec = sizeof(Vector3) / sizeof(float);
    bool skinning = it_s != vertices.end();

    Skeleton* skeleton = object && object->getType() == eOBJECT_MODEL ? static_cast<Model*>(object)->getSkeleton() : NULL;

    float* buffer = mapBuffer();
    float* physics = Physics::getInstance()->mapShape(object->getCollisionMesh() == this ? object : NULL);

    for (int i = 0; i < getVerticesCount(); i++)
    {
        float* v = buffer + i * inc;

        float& px = *v; v++;
        float& py = *v; v++;
        float& pz = *v;

        if (skinning && skeleton)
        {
            Vertex3D& skin = *it_s++;
            float w = 0.0f;
            Vector3 pos = Vector3(0.0f, 0.0f, 0.0f);
            Vector3 norm = Vector3(0.0f, 0.0f, 0.0f);
            Vector3 tan = Vector3(0.0f, 0.0f, 0.0f);

            bool havebone = false;
            for (int b = 0; b < M3D_MULTI_SKIN_COUNT; b++)
            {
                Bone* bone = skeleton->getBone(skin.bones.getElem(b));
                if (bone)
                {
                    pos += Vector4(bone->getMatrixSkin() * Vector4(skin.position, 1.0f)).getXYZ() * skin.weights.getElem(b);
                    norm += Vector4(bone->getMatrixSkin() * Vector4(skin.normal, 0.0f)).getXYZ() * skin.weights.getElem(b);
                    tan += Vector4(bone->getMatrixSkin() * Vector4(skin.tangent, 0.0f)).getXYZ() * skin.weights.getElem(b);
                    w += skin.weights.getElem(b);

                    havebone = true;
                }
                else
                {
                    break;
                }
            }

            if (havebone)
            {
                v = buffer + i * inc + vec;

                float& nx = *v; v++;
                float& ny = *v; v++;
                float& nz = *v;

                v = buffer + i * inc + vec * 2;

                float& tx = *v; v++;
                float& ty = *v; v++;
                float& tz = *v;

                if (w != 0.0f)
                {
                    w = 1.0f / w;
                }

                if (w != 1.0f)
                {
                    pos *= w;
                    norm *= w;
                    tan *= w;
                }

                px = pos.getX();
                py = pos.getY();
                pz = pos.getZ();

                nx = norm.getX();
                ny = norm.getY();
                nz = norm.getZ();

                tx = tan.getX();
                ty = tan.getY();
                tz = tan.getZ();

                if (physics)
                {
                    float* ph = physics + i * incp;

                    float& phx = *ph; ph++;
                    float& phy = *ph; ph++;
                    float& phz = *ph; ph++;

                    phx = px;
                    phy = py;
                    phz = pz;
                }
            }
        }

        if (first)
        {
            min = Vector3(px, py, pz);
            max = Vector3(px, py, pz);
            first = false;
        }
        else
        {
            min.setX(Math::min(px, min.getX()));
            min.setY(Math::min(py, min.getY()));
            min.setZ(Math::min(pz, min.getZ()));

            max.setX(Math::max(px, max.getX()));
            max.setY(Math::max(py, max.getY()));
            max.setZ(Math::max(pz, max.getZ()));
        }

        box.corners[0] = min;
        box.corners[1] = max;
    }

    Physics::getInstance()->unmapShape(object);
    unmapBuffer();
#else
    float* physics = Physics::getInstance()->mapShape(object->getCollisionMesh() == this ? object : NULL);

    if (!computedBox || physics)
    {
        computedBox = true;
        bool first = true;
        Vector3 min = Vector3(0.0f, 0.0f, 0.0f);
        Vector3 max = Vector3(0.0f, 0.0f, 0.0f);
        box.corners[0] = min;
        box.corners[1] = max;

        int incp = sizeof(btVector3) / sizeof(float);

        Skeleton* skeleton = object && object->getType() == eOBJECT_MODEL ? static_cast<Model*>(object)->getSkeleton() : NULL;

        for (int i = 0; i < getVerticesCount(); i++)
        {
            Vertex3D& vertex = vertices[i];
            Vector3 pos = vertex.position;

            if (skinned && skeleton)
            {
                float w = 0.0f;

                Vector3 bonePos = Vector3(0.0f, 0.0f, 0.0f);

                bool havebone = false;
                for (int b = 0; b < M3D_MULTI_SKIN_COUNT; b++)
                {
                    Bone* bone = skeleton->getBone(vertex.bones.getElem(b));
                    if (bone)
                    {
                        bonePos += Vector4(bone->getMatrixSkin() * Vector4(vertex.position, 1.0f)).getXYZ() * vertex.weights.getElem(b);
                        w += vertex.weights.getElem(b);

                        havebone = true;
                    }
                    else
                    {
                        break;
                    }
                }

                if (havebone)
                {
                    if (w != 0.0f)
                    {
                        w = 1.0f / w;
                    }

                    if (w != 1.0f)
                    {
                        bonePos *= w;
                    }

                    pos = bonePos;

                    if (physics)
                    {
                        float* ph = physics + i * incp;

                        float& phx = *ph; ph++;
                        float& phy = *ph; ph++;
                        float& phz = *ph; ph++;

                        phx = pos.getX();
                        phy = pos.getY();
                        phz = pos.getZ();
                    }
                }
            }

            if (first)
            {
                min = Vector3(pos.getX(), pos.getY(), pos.getZ());
                max = Vector3(pos.getX(), pos.getY(), pos.getZ());
                first = false;
            }
            else
            {
                min.setX(Math::min(pos.getX(), min.getX()));
                min.setY(Math::min(pos.getY(), min.getY()));
                min.setZ(Math::min(pos.getZ(), min.getZ()));

                max.setX(Math::max(pos.getX(), max.getX()));
                max.setY(Math::max(pos.getY(), max.getY()));
                max.setZ(Math::max(pos.getZ(), max.getZ()));
            }

            box.corners[0] = min;
            box.corners[1] = max;
        }

#ifndef _MGE_
        if (skeleton)
        {
            float maxSize = box.getMaxSize() * 0.625f;
            Vector3 center = box.getCenter();
            Vector3 boxMax = Vector3(maxSize, maxSize, maxSize);
            box.corners[0] = center - boxMax;
            box.corners[1] = center + boxMax;
        }
#endif
        Physics::getInstance()->unmapShape(object);
    }
#endif

    return true;
}

void Magic3D::MeshData::setType(MESH type)
{
    this->type = type;
}

Magic3D::MESH Magic3D::MeshData::getType()
{
    return type;
}

const std::string& Magic3D::MeshData::getName()
{
    return name;
}

void Magic3D::MeshData::setShape(btCollisionShape* shape)
{
    if (this->shape)
    {
        delete this->shape;
        this->shape = NULL;
    }
    this->shape = shape;
}

btCollisionShape* Magic3D::MeshData::getShape()
{
    return shape;
}

void Magic3D::MeshData::setTriangleMesh(btTriangleMesh* triangleMesh)
{
    if (this->triangleMesh)
    {
        delete this->triangleMesh;
        this->triangleMesh = NULL;
    }
    this->triangleMesh = triangleMesh;
}

btTriangleMesh* Magic3D::MeshData::getTriangleMesh()
{
    return triangleMesh;
}

std::vector<Magic3D::Vertex3D>* Magic3D::MeshData::getVertices()
{
    return &vertices;
}

std::vector<vindex>* Magic3D::MeshData::getIndexes()
{
    return &indexes;
}

std::vector<Magic3D::LineIndexes>* Magic3D::MeshData::getLines()
{
    return &lines;
}

std::vector<vindex>* Magic3D::MeshData::getPoints()
{
    return &points;
}

int Magic3D::MeshData::getVerticesCount()
{
    int result = 0;

    if (Renderer::getInstance()->hasMapBuffer() && isVBO())
    {
        result = renderID.dataSize / sizeof(Vertex3D);
    }
    else
    {
        result = vertices.size();
    }

    return result;
}

int Magic3D::MeshData::getIndexesCount()
{
    int result = 0;

    if (Renderer::getInstance()->hasMapBuffer() && isVBO())
    {
        result = renderID.indexSize / sizeof(vindex);
    }
    else
    {
        result = indexes.size();
    }

    return result;
}

int Magic3D::MeshData::getTrianglesCount()
{
    int result = 0;

    if (Renderer::getInstance()->hasMapBuffer() && isVBO())
    {
        result = renderID.indexSize / sizeof(vindex);
    }
    else
    {
        result = indexes.size();
    }

    if (getType() == eMESH_TRIANGLES_STRIP)
    {
        result = result - 2;
    }
    else
    {
        result = result / 3;
    }


    return result;
}

int Magic3D::MeshData::getLinesCount()
{
    return lines.size();
}

int Magic3D::MeshData::getPointsCount()
{
    return points.size();
}

void Magic3D::MeshData::addVertex(Vertex3D vertex)
{
    vertices.push_back(vertex);
}

void Magic3D::MeshData::addIndex(vindex index)
{
    indexes.push_back(index);
}

void Magic3D::MeshData::addTriangle(TriangleIndexes triangle)
{
    //triangles.push_back(triangle);
    indexes.push_back(triangle.index1);
    indexes.push_back(triangle.index2);
    indexes.push_back(triangle.index3);
#if defined(_MGE_) || defined(DEBUG)
    lines.push_back(LineIndexes(triangle.index1, triangle.index2));
    lines.push_back(LineIndexes(triangle.index1, triangle.index3));
    lines.push_back(LineIndexes(triangle.index2, triangle.index3));
#endif
    updateTangent(triangle.index1, triangle.index2, triangle.index3);
}

void Magic3D::MeshData::updateTangent(vindex index0, vindex index1, vindex index2)
{
    MeshData::updateTangent((float*)&vertices.front(), vertices.size(), index0, index1, index2);
}

int Magic3D::MeshData::addQuad(float x, float y, float width, float height, AXIS axis, bool inverse)
{
    return addQuad(x, y, 0.0f, width, height, axis, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f), inverse);
}

int Magic3D::MeshData::addQuad(float x, float y, float width, float height, AXIS axis, ColorRGBA color, bool inverse)
{
    return addQuad(x, y, 0.0f, width, height, axis, color, inverse);
}

int Magic3D::MeshData::addQuad(float x, float y, float z, float width, float height, AXIS axis, bool inverse)
{
    return addQuad(x, y, z, width, height, axis, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f), inverse);
}

int Magic3D::MeshData::addQuad(float x, float y, float z, float width, float height, AXIS axis, ColorRGBA color, bool inverse)
{
    float widthHalf  = width * 0.5f;
    float heightHalf = height * 0.5f;

    switch (axis)
    {
        case eAXIS_X:
        {
            Vector3 normal(x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
            addVertex(Vertex3D(Vector3(x, y - heightHalf, z - widthHalf), normal, color, Texture2D(0.0f, 1.0f)));
            addVertex(Vertex3D(Vector3(x, y + heightHalf, z - widthHalf), normal, color, Texture2D(0.0f, 0.0f)));
            addVertex(Vertex3D(Vector3(x, y + heightHalf, z + widthHalf), normal, color, Texture2D(1.0f, 0.0f)));
            addVertex(Vertex3D(Vector3(x, y - heightHalf, z + widthHalf), normal, color, Texture2D(1.0f, 1.0f)));
            break;
        }
        case eAXIS_Y:
        {
            Vector3 normal(0.0f, y < 0.0f ? -1.0f : 1.0f, 0.0f);
            addVertex(Vertex3D(Vector3(x - widthHalf, y, z - heightHalf), normal, color, Texture2D(0.0f, 1.0f)));
            addVertex(Vertex3D(Vector3(x - widthHalf, y, z + heightHalf), normal, color, Texture2D(0.0f, 0.0f)));
            addVertex(Vertex3D(Vector3(x + widthHalf, y, z + heightHalf), normal, color, Texture2D(1.0f, 0.0f)));
            addVertex(Vertex3D(Vector3(x + widthHalf, y, z - heightHalf), normal, color, Texture2D(1.0f, 1.0f)));
            break;
        }
        case eAXIS_Z:
        {
            Vector3 normal(0.0f, 0.0f, z < 0.0f ? -1.0f : 1.0f);
            addVertex(Vertex3D(Vector3(x - widthHalf, y - heightHalf, z), normal, color, Texture2D(0.0f, 1.0f)));
            addVertex(Vertex3D(Vector3(x - widthHalf, y + heightHalf, z), normal, color, Texture2D(0.0f, 0.0f)));
            addVertex(Vertex3D(Vector3(x + widthHalf, y + heightHalf, z), normal, color, Texture2D(1.0f, 0.0f)));
            addVertex(Vertex3D(Vector3(x + widthHalf, y - heightHalf, z), normal, color, Texture2D(1.0f, 1.0f)));
            break;
        }
    }
    int index = getVertices()->size() - 4;
    if (inverse)
    {
        addTriangle(TriangleIndexes(index + 3, index + 0, index + 2));
        addTriangle(TriangleIndexes(index + 2, index + 0, index + 1));
    }
    else
    {
        addTriangle(TriangleIndexes(index + 0, index + 1, index + 3));
        addTriangle(TriangleIndexes(index + 3, index + 1, index + 2));
    }

    return index;
}

void Magic3D::MeshData::setQuad(float* buffer, int index, float x, float y, float width, float height, ColorRGBA color)
{
    setQuad(buffer, index, x, y, 0.0f, width, height, color);
}

void Magic3D::MeshData::setQuad(float* buffer, int index, float x, float y, float z, float width, float height, ColorRGBA color)
{
    Vector3 v0;
    Vector3 v1;
    Vector3 v2;
    Vector3 v3;

    float startX = x;
    float startY = y;

    v0 = Vector3(startX, startY, z);

    startY += height;
    v1 = Vector3(startX, startY, z);

    startX += width;
    v2 = Vector3(startX, startY, z);

    startY -= height;
    v3 = Vector3(startX, startY, z);

    setQuad(buffer, index, v0, v1, v2, v3, color);
}

void Magic3D::MeshData::setQuad(float* buffer, int index, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, const Vector3& vertex4, ColorRGBA& color)
{
    setQuad(buffer, index, vertex1, vertex2, vertex3, vertex4, color, color, color, color);
}

void Magic3D::MeshData::setQuad(float* buffer, int index, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, const Vector3& vertex4, ColorRGBA& color1, ColorRGBA& color2, ColorRGBA& color3, ColorRGBA& color4)
{
    if (buffer)
    {
        const Vector3* v[4];
        v[0] = &vertex1;
        v[1] = &vertex2;
        v[2] = &vertex3;
        v[3] = &vertex4;

        const ColorRGBA* c[4];
        c[0] = &color1;
        c[1] = &color2;
        c[2] = &color3;
        c[3] = &color4;

        for (int i = 0; i < 4; i++)
        {
            int stride = index * 4 + i;
            int inc = sizeof(Vertex3D) / sizeof(float);
            float* b = buffer + stride * inc;

            *b = v[i]->getX(); b++;
            *b = v[i]->getY(); b++;
            *b = v[i]->getZ();

            b = buffer + stride * inc + (sizeof(Vector3) / sizeof(float)) * 3;

            *b = c[i]->r; b++;
            *b = c[i]->g; b++;
            *b = c[i]->b; b++;
            *b = c[i]->a;
        }
    }
}


void Magic3D::MeshData::setQuadTexture(float* buffer, int index, float x, float y, float width, float height)
{
    for (int i = 0; i < M3D_MULTI_TEXTURE_COUNT; i++)
    {
        setQuadTexture(buffer, index, i, x, y, width, height);
    }
}

void Magic3D::MeshData::setQuadTexture(float* buffer, int index, int uvIndex, float x, float y, float width, float height)
{
    Texture2D v0;
    Texture2D v1;
    Texture2D v2;
    Texture2D v3;

    float startX = x;
    float startY = y;

    v0 = Texture2D(startX, startY);

    startY -= height;
    v1 = Texture2D(startX, startY);

    startX += width;
    v2 = Texture2D(startX, startY);

    startY += height;
    v3 = Texture2D(startX, startY);

    setQuadTexture(buffer, index, uvIndex, v0, v1, v2, v3);
}

void Magic3D::MeshData::setQuadTexture(float* buffer, int index, const Texture2D& uv1, const Texture2D& uv2, const Texture2D& uv3, const Texture2D& uv4)
{
    for (int i = 0; i < M3D_MULTI_TEXTURE_COUNT; i++)
    {
        setQuadTexture(buffer, index, i, uv1, uv2, uv3, uv4);
    }
}

void Magic3D::MeshData::setQuadTexture(float* buffer, int index, int uvIndex, const Texture2D& uv1, const Texture2D& uv2, const Texture2D& uv3, const Texture2D& uv4)
{
    if (buffer)
    {
        const Texture2D* v[4];
        v[0] = &uv1;
        v[1] = &uv2;
        v[2] = &uv3;
        v[3] = &uv4;

        for (int i = 0; i < 4; i++)
        {
            int stride = index * 4 + i;
            int inc = sizeof(Vertex3D) / sizeof(float);
            int tex = sizeof(Texture2D) / sizeof(float);

            float* b = buffer + stride * inc + (sizeof(Vector3) / sizeof(float)) * 3 + (sizeof(ColorRGBA) / sizeof(float)) + tex * uvIndex;

            *b = v[i]->u; b++;
            *b = v[i]->v; b++;
        }
    }
}

void Magic3D::MeshData::moveQuad(float* buffer, int index, float x, float y, float z)
{
    if (buffer)
    {
        for (int i = 0; i < 4; i++)
        {
            int stride = index * 4 + i;
            int inc = sizeof(Vertex3D) / sizeof(float);
            float* b = buffer + stride * inc;

            *b += x; b++;
            *b += y; b++;
            *b += z; b++;
        }
    }
}

void Magic3D::MeshData::removeQuad(int index)
{
    int idx = index * 4;
    std::vector<Magic3D::Vertex3D>::iterator vertex = getVertices()->begin() + idx + 3;
    for (int i = 0; i < 4; i++)
    {
        if (getVertices()->size() > 0)
        {
            getVertices()->erase(vertex--);
        }
    }

    idx = index * 6;
    std::vector<vindex>::iterator triangle = getIndexes()->begin() + idx + 3;
    for (int i = 0; i < 2; i++)
    {
        if (getIndexes()->size() > 0)
        {
            getIndexes()->erase(triangle--);
            getIndexes()->erase(triangle--);
            getIndexes()->erase(triangle--);
        }
    }
}

void Magic3D::MeshData::addLine(Vector3 start, Vector3 finish, ColorRGBA color)
{
    addVertex(Vertex3D(start, Vector3(0.0f, 1.0f, 0.0f), color, Texture2D(0.0f, 0.0f)));
    addVertex(Vertex3D(finish, Vector3(0.0f, 1.0f, 0.0f), color, Texture2D(0.0f, 0.0f)));

    lines.push_back(LineIndexes(vertices.size() - 2, vertices.size() - 1));
}

void Magic3D::MeshData::addPoint(Vector3 position, float size, ColorRGBA color)
{
    addVertex(Vertex3D(position, Vector3(0.0f, 1.0f, 0.0f), color, Texture2D(size, 0.0f)));

    points.push_back(vertices.size() - 1);
}

void Magic3D::MeshData::setSkinned(bool skinned)
{
    this->skinned = skinned;
}

bool Magic3D::MeshData::isSkinned()
{
    return skinned;
}

void Magic3D::MeshData::setRenderID(RENDER_ID id)
{
    renderID = id;
}

const Magic3D::RENDER_ID& Magic3D::MeshData::getRenderID()
{
    return renderID;
}

bool Magic3D::MeshData::isVBO()
{
    return renderID.data > 0 && renderID.index > 0;
}

void Magic3D::MeshData::createVbo()
{
    Renderer* renderer = Renderer::getInstance();
    if (renderer)
    {
        deleteVbo();
        if (renderer->createVBO(this))
        {
            if (Renderer::getInstance()->hasMapBuffer())
            {
#if !defined(MAGIC3D_LEGACY) //CPU Skinning
                vertices.clear();
#endif
                indexes.clear();
            }
        }
    }
}

void Magic3D::MeshData::deleteVbo()
{
    Renderer* renderer = Renderer::getInstance();
    if (renderer)
    {
        renderer->deleteVBO(renderID);
    }

    renderID.id = 0;
    renderID.data = 0;
    renderID.index = 0;
    renderID.dataSize = 0;
    renderID.indexSize = 0;

    computedBox = false;
}

float* Magic3D::MeshData::mapBuffer()
{
    float* result = (float*)&vertices.front();

    if (Renderer::getInstance()->hasMapBuffer() && isVBO())
    {
        result = Renderer::getInstance()->mapVBO(getRenderID().data);
    }

    return result;
}

void Magic3D::MeshData::unmapBuffer()
{
    if (!Renderer::getInstance()->hasMapBuffer())
    {
        Renderer::getInstance()->updateVBO(this);
    }
    else if (isVBO())
    {
        Renderer::getInstance()->unmapVBO();
    }
}

vindex* Magic3D::MeshData::mapIndexes()
{
    vindex* result = (vindex*)&indexes.front();

    if (Renderer::getInstance()->hasMapBuffer() && isVBO())
    {
        result = Renderer::getInstance()->mapIndexes(getRenderID().index);
    }

    return result;
}

void Magic3D::MeshData::unmapIndexes()
{
    if (Renderer::getInstance()->hasMapBuffer() && isVBO())
    {
        Renderer::getInstance()->unmapIndexes();
    }
}

const Magic3D::Box& Magic3D::MeshData::getBoundingBox()
{
    return box;
}

void Magic3D::MeshData::clear()
{
    points.clear();
    lines.clear();
    indexes.clear();
    vertices.clear();
}

void Magic3D::MeshData::createPlaneStrip(std::vector<vindex>* indexes, std::vector<LineIndexes>* lines, int length, int width)
{
    vindex q0 = 0;
    vindex q1 = 0;
    vindex q2 = 0;
    vindex q3 = 0;

    for (int r = 1; r < length; r++)
    {
        for (int c = 0; c < width ; c++)
        {
            bool even = r % 2 == 0;
            if (even) // <- direction
            {
                q0 = width * r - (c + 1);
                q1 = width * r + width - (c + 1);
                q2 = width * r - (c + 2);
                q3 = width * r + width - (c + 2);
            }
            else // -> direction
            {
                q0 = width * r - width + c;
                q1 = width * r + c;
                q2 = width * r - width + c + 1;
                q3 = width * r + c + 1;
            }

            if (c == 0)
            {
                indexes->push_back(q0);
            }

            indexes->push_back(q1);

            if (c == width - 1)
            {
                indexes->push_back(q1);
            }

            if (c < width - 1)
            {
                indexes->push_back(q2);

                lines->push_back(LineIndexes(q0, q2));
                lines->push_back(LineIndexes(q1, q2));

                if (r == length - 1)
                {
                    lines->push_back(LineIndexes(q1, q3));
                }
            }

            lines->push_back(LineIndexes(q0, q1));
        }
    }
}

void Magic3D::MeshData::updatePlaneStripNormals(float* vertices, int vcount, vindex* indexes, int icount)    
{
    vindex q0 = 0;
    vindex q1 = 0;
    vindex q2 = 0;

    int incv = sizeof(Vertex3D) / sizeof(float);
    int inc = sizeof(Vector3) / sizeof(float);

    for (int i = 1; i < icount; i++)
    {
        if (i % 2 == 0)
        {
            for (int a = 0; a < 2; a++)
            {
                int index = i;
                if (a == 1)
                {
                    if (index + 1 >= icount)
                    {
                        continue;
                    }
                    q0 = *(indexes + index);
                    q1 = *(indexes + (index - 1));
                    q2 = *(indexes + (index + 1));
                }
                else
                {
                    q0 = *(indexes + (index - 2));
                    q1 = *(indexes + (index - 1));
                    q2 = *(indexes + index);
                }

                if (q0 < (vindex)vcount && q1 < (vindex)vcount && q2 < (vindex)vcount && q0 != q1 && q0 != q2 && q1 != q2)
                {
                    float* v = vertices + incv * q0;
                    Vector3 Ap;
                    Vector3 An;
                    Ap.setX(*v); v++;
                    Ap.setY(*v); v++;
                    Ap.setZ(*v);
                    v = vertices + incv * q0 + inc;
                    An.setX(*v); v++;
                    An.setY(*v); v++;
                    An.setZ(*v);
                    v = vertices + incv * q1;
                    Vector3 Bp;
                    Vector3 Bn;
                    Bp.setX(*v); v++;
                    Bp.setY(*v); v++;
                    Bp.setZ(*v);
                    v = vertices + incv * q1 + inc;
                    Bn.setX(*v); v++;
                    Bn.setY(*v); v++;
                    Bn.setZ(*v);
                    v = vertices + incv * q2;
                    Vector3 Cp;
                    Vector3 Cn;
                    Cp.setX(*v); v++;
                    Cp.setY(*v); v++;
                    Cp.setZ(*v);
                    v = vertices + incv * q2 + inc;
                    Cn.setX(*v); v++;
                    Cn.setY(*v); v++;
                    Cn.setZ(*v); v++;

                    Vector3 N = cross(Bp - Ap, Cp - Ap);
                    float sin_alpha = asin(Vectormath::Aos::length(N) / (Vectormath::Aos::length(Bp - Ap) * Vectormath::Aos::length(Cp - Ap)));


                    Vector3 normal = normalize(N) * (sin_alpha > 0.0f ? sin_alpha : 1.0f);

                    Vector3 NA = normalize(An + normal);
                    Vector3 NB = normalize(Bn + normal);
                    Vector3 NC = normalize(Cn + normal);

                    v = vertices + incv * q0 + inc;
                    *v = NA.getX(); v++;
                    *v = NA.getY(); v++;
                    *v = NA.getZ();
                    v = vertices + incv * q1 + inc;
                    *v = NB.getX(); v++;
                    *v = NB.getY(); v++;
                    *v = NB.getZ();
                    v = vertices + incv * q2 + inc;
                    *v = NC.getX(); v++;
                    *v = NC.getY(); v++;
                    *v = NC.getZ(); v++;

                    updateTangent(vertices, vcount, q0, q1, q2);
                }
            }
        }
    }
}

void Magic3D::MeshData::updateTangent(float* vertices, int vcount, vindex index0, vindex index1, vindex index2)
{
    int inc = sizeof(Vertex3D) / sizeof(float);
    int incv = sizeof(Vector3) / sizeof(float);
    int incc = sizeof(ColorRGBA) / sizeof(float);

    Vertex3D vs[3];
    vindex indexes[3] = {index0, index1, index2};

    for (int i = 0; i < 3; i++)
    {
        int index = indexes[i];
        if (index < vcount)
        {
            float* v = vertices + inc * index;
            vs[i].position.setX(*v); v++;
            vs[i].position.setY(*v); v++;
            vs[i].position.setZ(*v);
            v = vertices + inc * index + incv;
            vs[i].normal.setX(*v); v++;
            vs[i].normal.setY(*v); v++;
            vs[i].normal.setZ(*v);
            v = vertices + inc * index + incv * 2;
            vs[i].tangent.setX(*v); v++;
            vs[i].tangent.setY(*v); v++;
            vs[i].tangent.setZ(*v);
            v = vertices + inc * index + incv * 3 + incc;
            vs[i].uv[0].u = *v; v++;
            vs[i].uv[0].v = *v; v++;
        }
    }

    // Edges of the triangle : postion delta
    Vector3 deltaPos1 = vs[1].position - vs[0].position;
    Vector3 deltaPos2 = vs[2].position - vs[0].position;

    // UV delta
    Texture2D deltaUV1 = Texture2D(vs[1].uv[0].u - vs[0].uv[0].u, vs[1].uv[0].v - vs[0].uv[0].v);
    Texture2D deltaUV2 = Texture2D(vs[2].uv[0].u - vs[0].uv[0].u, vs[2].uv[0].v - vs[0].uv[0].v);

    float r = 1.0f / (deltaUV1.u * deltaUV2.v - deltaUV1.v * deltaUV2.u);
    Vector3 tangent = Vector3(deltaPos1 * deltaUV2.v - deltaPos2 * deltaUV1.v) * r;

    for (unsigned int i = 0; i < 3; i++ )
    {
        int index = indexes[i];
        float* v = vertices + inc * index + incv * 2;
        *v = tangent.getX(); v++;
        *v = tangent.getY(); v++;
        *v = tangent.getZ(); v++;
    }

    for (unsigned int i = 0; i < 3; i++ )
    {
        Vertex3D vc;
        int index = indexes[i];
        if (index < vcount)
        {
            float* v = vertices + inc * index + incv;
            vc.normal.setX(*v); v++;
            vc.normal.setY(*v); v++;
            vc.normal.setZ(*v);
            v = vertices + inc * index + incv * 2;
            vc.tangent.setX(*v); v++;
            vc.tangent.setY(*v); v++;
            vc.tangent.setZ(*v); v++;

            Vector3 n = vc.normal;
            Vector3 t = vc.tangent;
            Vector3 b = normalize(cross(n, t));

            // Gram-Schmidt orthogonalize
            t = normalize(t - n * dot(n, t));

            // Calculate handedness
            if (dot(cross(n, t), b) < 0.0f){
                t = t * -1.0f;
            }

            v = vertices + inc * index + incv * 2;
            *v = t.getX(); v++;
            *v = t.getY(); v++;
            *v = t.getZ(); v++;
        }
    }
}

Magic3D::XMLElement* Magic3D::MeshData::save(XMLElement* root)
{
    if (root)
    {

    }
    return root;
}

Magic3D::XMLElement* Magic3D::MeshData::load(XMLElement* root)
{
    if (root)
    {

    }
    return root;
}

//******************************************************************************
Magic3D::Mesh::Mesh(const Mesh& mesh)
{
    this->owner = mesh.owner;
    this->doubleSide = mesh.doubleSide;
    this->visible = mesh.visible;
    this->illuminated = mesh.illuminated;
    this->fog = mesh.fog;
    this->reflective = mesh.reflective;
    this->glow = mesh.glow;
    this->castShadows = mesh.castShadows;
    this->receiveShadows = mesh.receiveShadows;

    std::vector<Material*>::const_iterator it_m = mesh.materials.begin();

    while (it_m != mesh.materials.end())
    {
        Material* material = *it_m++;
        materials.push_back(material);
    }

    data = mesh.data && mesh.owner ? static_cast<MeshData*>(mesh.data->spawn()) : mesh.data;
}

Magic3D::Mesh::Mesh(MESH type, std::string name, bool doubleSide)
{
    init(NULL, true, doubleSide, type, name);
}

Magic3D::Mesh::Mesh(MeshData* data, bool doubleSide, bool owner)
{
    init(data, owner, doubleSide);
}

void Magic3D::Mesh::init(MeshData* data, bool owner, bool doubleSide, MESH type, std::string name)
{
    if (data)
    {
        this->owner = owner;
        this->data = data;
    }
    else
    {
        this->owner = true;
        this->data = new MeshData(type, name);
    }
    this->doubleSide = doubleSide;
    visible = true;
    illuminated = true;
    fog = true;
    reflective = false;
    glow = false;
    castShadows = true;
    receiveShadows = true;
}

Magic3D::Mesh::~Mesh()
{
    if (owner && data)
    {
        delete data;
        data = NULL;
    }
}

Magic3D::Mesh* Magic3D::Mesh::spawn() const
{
    return (new Mesh(*this));
}

Magic3D::MeshData* Magic3D::Mesh::getData()
{
    return data;
}

bool Magic3D::Mesh::update(Object* object)
{
    bool result = true;
    if (data)
    {
        result = data->update(object);
    }
    return result;
}

std::vector<Magic3D::Material*>* Magic3D::Mesh::getMaterials()
{
    return &materials;
}

void Magic3D::Mesh::addMaterial(Material* material)
{
    if (material)
    {
        materials.push_back(material);
    }
}

void Magic3D::Mesh::setDoubleSide(bool doubleSide)
{
    this->doubleSide = doubleSide;
}

bool Magic3D::Mesh::isDoubleSide()
{
    return doubleSide;
}

void Magic3D::Mesh::setIlluminated(bool illuminated)
{
    this->illuminated = illuminated;
}

bool Magic3D::Mesh::isIlluminated()
{
    return illuminated;
}

void Magic3D::Mesh::setFogged(bool fog)
{
    this->fog = fog;
}

bool Magic3D::Mesh::isFogged()
{
    return fog;
}

void Magic3D::Mesh::setReflective(bool reflective)
{
    this->reflective = reflective;
}

bool Magic3D::Mesh::isReflective()
{
    return reflective;
}

void Magic3D::Mesh::setGlow(bool glow)
{
    this->glow = glow;
}

bool Magic3D::Mesh::isGlow()
{
    return glow;
}

void Magic3D::Mesh::setVisible(bool visible)
{
    this->visible = visible;
}

bool Magic3D::Mesh::isVisible()
{
    return visible;
}

void Magic3D::Mesh::setCastShadows(bool cast)
{
    castShadows = cast;
}

bool Magic3D::Mesh::isCastingShadows()
{
    return castShadows;
}

void Magic3D::Mesh::setReceiveShadows(bool receive)
{
    receiveShadows = receive;
}

bool Magic3D::Mesh::isReceivingShadows()
{
    return receiveShadows;
}

Magic3D::XMLElement* Magic3D::Mesh::save(XMLElement* root)
{
    if (root)
    {
        saveString(root, M3D_MESH_XML_NAME,            getData()->getName());
        saveInt(   root, M3D_MESH_XML_TYPE,            getData()->getType());
        saveInt(   root, M3D_MESH_XML_VISIBLE,         isVisible() ? 1 : 0);
        saveInt(   root, M3D_MESH_XML_DOUBLE_SIDE,     isDoubleSide() ? 1 : 0);
        saveInt(   root, M3D_MESH_XML_ILLUMINATED,     isIlluminated() ? 1 : 0);
        saveInt(   root, M3D_MESH_XML_FOG,             isFogged() ? 1 : 0);
        saveInt(   root, M3D_MESH_XML_REFLECTIVE,      isReflective() ? 1 : 0);
        saveInt(   root, M3D_MESH_XML_GLOW,            isGlow() ? 1 : 0);
        saveInt(   root, M3D_MESH_XML_CAST_SHADOWS,    isCastingShadows() ? 1 : 0);
        saveInt(   root, M3D_MESH_XML_RECEIVE_SHADOWS, isReceivingShadows() ? 1 : 0);

        std::vector<Material*>::const_iterator it_m = materials.begin();
        while (it_m != materials.end())
        {
            Material* material = *it_m++;

            saveString(root, M3D_MESH_XML_MATERIAL, material->getName());
        }
    }
    return root;
}

Magic3D::XMLElement* Magic3D::Mesh::load(XMLElement* root)
{
    if (root)
    {
        getData()->setType(MESH(loadInt(root, M3D_MESH_XML_TYPE)));
        visible        = loadInt(root, M3D_MESH_XML_VISIBLE) == 0 ? false : true;
        doubleSide     = loadInt(root, M3D_MESH_XML_DOUBLE_SIDE) == 0 ? false : true;
        illuminated    = loadInt(root, M3D_MESH_XML_ILLUMINATED) == 0 ? false : true;
        fog            = loadInt(root, M3D_MESH_XML_FOG) == 0 ? false : true;
        reflective     = loadInt(root, M3D_MESH_XML_REFLECTIVE) == 0 ? false : true;
        glow           = loadInt(root, M3D_MESH_XML_GLOW) == 0 ? false : true;
        castShadows    = loadInt(root, M3D_MESH_XML_CAST_SHADOWS) == 0 ? false : true;
        receiveShadows = loadInt(root, M3D_MESH_XML_RECEIVE_SHADOWS) == 0 ? false : true;

        materials.clear();
        XMLElement* xml = root->FirstChildElement(M3D_MESH_XML_MATERIAL);
        while (xml)
        {
            std::string materialName = xml->GetText();
            bool created = false;
            addMaterial(ResourceManager::getMaterials()->load(materialName, created));
            xml = xml->NextSiblingElement(M3D_MESH_XML_MATERIAL);
        }
    }
    return root;
}
