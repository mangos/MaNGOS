
#include "Debug.h"

#include <vector>
#include "DetourNavMesh.h"
#include "Recast.h"

#include <string>
using namespace std;

void duReadNavMesh(char* tile, dtNavMesh* &navMesh)
{
    FILE* file;

    string tileName(tile);
    tileName = tileName.substr(0, tileName.find_first_of('.'));
    string mapName = "mmaps/" + tileName.substr(0, 3) + ".mmap";
    file = fopen(mapName.c_str(), "rb");

    if(!file)
        return;

    dtNavMeshParams params;
    fread(&params, sizeof(dtNavMeshParams), 1, file);
    fclose(file);

    if (!navMesh)
    {
        dtFreeNavMesh(navMesh);
        navMesh = dtAllocNavMesh();
        navMesh->init(&params);
    }

    int map = atoi(tileName.substr(0, 3).c_str());
    int x = atoi(tileName.substr(5, 2).c_str());
    int y = atoi(tileName.substr(3, 2).c_str());
    bool loaded = false;

    char fname[50];

    int count = 0;
    for (int i = x-1; i <= x+1; ++i)
        for (int j = y-1; j <= y+1; ++j)
    //int i = x;
    //int j = y;
        {
            sprintf(fname, "mmaps/%03i%02i%02i.mmtile", map, j, i);
            file = fopen(fname, "rb");
            if(file)
            {
                MmapTileHeader header;
                fread(&header, sizeof(MmapTileHeader), 1, file);

                unsigned char* data = (unsigned char*)dtAlloc(header.size, DT_ALLOC_PERM);
                fread(data, header.size, 1, file);

                dtStatus status = navMesh->addTile(data, header.size, DT_TILE_FREE_DATA, 0 , NULL);

                if (status != DT_SUCCESS)
                    dtFree(data);
                else
                    count++;

                fclose(file);
            }
        }

    if (!count)
    {
        dtFreeNavMesh(navMesh);
        navMesh = NULL;
    }
}

int duReadHeightfield(char* tile, rcHeightfield* &hf)
{
    FILE* file;

    string tileName(tile);
    tileName = tileName.substr(0, tileName.find_first_of('.'));
    file = fopen(("meshes/" + tileName + ".hf").c_str(), "rb");
    
    if(!file)
        return false;

    hf = rcAllocHeightfield();

    fread(&(hf->cs), sizeof(float), 1, file);
    fread(&(hf->ch), sizeof(float), 1, file);
    fread(&(hf->width), sizeof(int), 1, file);
    fread(&(hf->height), sizeof(int), 1, file);
    fread(hf->bmin, sizeof(float), 3, file);
    fread(hf->bmax, sizeof(float), 3, file);

    hf->spans = new rcSpan*[hf->width*hf->height];
    memset(hf->spans, 0, sizeof(rcSpan*)*hf->width*hf->height);

    for(int y = 0; y < hf->height; ++y)
        for(int x = 0; x < hf->width; ++x)
        {
            int spanCount;
            fread(&spanCount, sizeof(int), 1, file);

            if(spanCount)
                hf->spans[x + y*hf->width] = new rcSpan;
            else
                hf->spans[x + y*hf->width] = NULL;

            rcSpan* span = hf->spans[x + y*hf->width];

            while(spanCount--)
            {
                fread(span, sizeof(rcSpan), 1, file);

                if(spanCount)
                {
                    span->next = new rcSpan;
                    span = span->next;
                }
                else
                    span->next = NULL;
            }
        }

    fclose(file);

    return false;
}

int duReadCompactHeightfield(char* tile, rcCompactHeightfield* &chf)
{
    FILE* file;

    string tileName(tile);
    tileName = tileName.substr(0, tileName.find_first_of('.'));
    file = fopen(("meshes/" + tileName + ".chf").c_str(), "rb");
    
    if(!file)
        return false;

    chf = rcAllocCompactHeightfield();

    fread(&chf->width, sizeof(chf->width), 1, file);
    fread(&chf->height, sizeof(chf->height), 1, file);
    fread(&chf->spanCount, sizeof(chf->spanCount), 1, file);

    fread(&chf->walkableHeight, sizeof(chf->walkableHeight), 1, file);
    fread(&chf->walkableClimb, sizeof(chf->walkableClimb), 1, file);

    fread(&chf->maxDistance, sizeof(chf->maxDistance), 1, file);
    fread(&chf->maxRegions, sizeof(chf->maxRegions), 1, file);

    fread(chf->bmin, sizeof(chf->bmin), 1, file);
    fread(chf->bmax, sizeof(chf->bmax), 1, file);
    
    fread(&chf->cs, sizeof(chf->cs), 1, file);
    fread(&chf->ch, sizeof(chf->ch), 1, file);

    int tmp = 0;
    fread(&tmp, sizeof(tmp), 1, file);

    if (tmp & 1)
    {
        chf->cells = new rcCompactCell[chf->width*chf->height];
        fread(chf->cells, sizeof(rcCompactCell), chf->width*chf->height, file);
    }
    if (tmp & 2)
    {
        chf->spans = new rcCompactSpan[chf->spanCount];
        fread(chf->spans, sizeof(rcCompactSpan), chf->spanCount, file);
    }
    if (tmp & 4)
    {
        chf->dist = new unsigned short[chf->spanCount];
        fread(chf->dist, sizeof(unsigned short), chf->spanCount, file);
    }
    if (tmp & 8)
    {
        chf->areas = new unsigned char[chf->spanCount];
        fread(chf->areas, sizeof(unsigned char), chf->spanCount, file);
    }

    fclose(file);

    return false;
}

int duReadContourSet(char* tile, rcContourSet* &cs)
{
    FILE* file;

    string tileName(tile);
    tileName = tileName.substr(0, tileName.find_first_of('.'));
    file = fopen(("meshes/" + tileName + ".cs").c_str(), "rb");
    
    if(!file)
        return false;

    cs = rcAllocContourSet();

    fread(&(cs->cs), sizeof(float), 1, file);
    fread(&(cs->ch), sizeof(float), 1, file);
    fread(cs->bmin, sizeof(float), 3, file);
    fread(cs->bmax, sizeof(float), 3, file);
    fread(&(cs->nconts), sizeof(int), 1, file);

    if(cs->nconts)
        cs->conts = new rcContour[cs->nconts];

    for(int j = 0; j < cs->nconts; ++j)
    {
        cs->conts[j].verts = 0;
        cs->conts[j].rverts = 0;

        fread(&(cs->conts[j].area), sizeof(unsigned char), 1, file);
        fread(&(cs->conts[j].reg), sizeof(unsigned short), 1, file);

        fread(&(cs->conts[j].nverts), sizeof(int), 1, file);
        cs->conts[j].verts = new int[cs->conts[j].nverts*4];
        fread(cs->conts[j].verts, sizeof(int), cs->conts[j].nverts*4, file);

        fread(&(cs->conts[j].nrverts), sizeof(int), 1, file);
        cs->conts[j].rverts = new int[cs->conts[j].nrverts*4];
        fread(cs->conts[j].rverts, sizeof(int), cs->conts[j].nrverts*4, file);
    }

    fclose(file);

    return false;
}

int duReadPolyMesh(char* tile, rcPolyMesh* &mesh)
{
    FILE* file;

    string tileName(tile);
    tileName = tileName.substr(0, tileName.find_first_of('.'));
    file = fopen(("meshes/" + tileName + ".pmesh").c_str(), "rb");
    
    if(!file)
        return false;

    mesh = rcAllocPolyMesh();

    fread(&(mesh->cs), sizeof(float), 1, file);
    fread(&(mesh->ch), sizeof(float), 1, file);
    fread(&(mesh->nvp), sizeof(int), 1, file);
    fread(mesh->bmin, sizeof(float), 3, file);
    fread(mesh->bmax, sizeof(float), 3, file);
    fread(&(mesh->nverts), sizeof(int), 1, file);
    mesh->verts = new unsigned short[mesh->nverts*3];
    fread(mesh->verts, sizeof(unsigned short), mesh->nverts*3, file);
    fread(&(mesh->npolys), sizeof(int), 1, file);
    mesh->polys = new unsigned short[mesh->npolys*mesh->nvp*2];
    mesh->flags = new unsigned short[mesh->npolys];
    mesh->areas = new unsigned char[mesh->npolys];
    mesh->regs = new unsigned short[mesh->npolys];
    fread(mesh->polys, sizeof(unsigned short), mesh->npolys*mesh->nvp*2, file);
    fread(mesh->flags, sizeof(unsigned short), mesh->npolys, file);
    fread(mesh->areas, sizeof(unsigned char), mesh->npolys, file);
    fread(mesh->regs, sizeof(unsigned short), mesh->npolys, file);

    fclose(file);

    return true;
}

int duReadDetailMesh(char* tile, rcPolyMeshDetail* &mesh)
{
    FILE* file;

    string tileName(tile);
    tileName = tileName.substr(0, tileName.find_first_of('.'));
    file = fopen(("meshes/" + tileName + ".dmesh").c_str(), "rb");
    
    if(!file)
        return false;

    mesh = rcAllocPolyMeshDetail();

    fread(&(mesh->nverts), sizeof(int), 1, file);
    mesh->verts = new float[mesh->nverts*3];
    fread(mesh->verts, sizeof(float), mesh->nverts*3, file);

    fread(&(mesh->ntris), sizeof(int), 1, file);
    mesh->tris = new unsigned char[mesh->ntris*4];
    fread(mesh->tris, sizeof(char), mesh->ntris*4, file);

    fread(&(mesh->nmeshes), sizeof(int), 1, file);
    mesh->meshes = new unsigned int[mesh->nmeshes*4];
    fread(mesh->meshes, sizeof(int), mesh->nmeshes*4, file);

    fclose(file);

    return true;
}

myMeshLoaderObj::myMeshLoaderObj() :
	m_verts(0),
	m_tris(0),
	m_normals(0),
	m_vertCount(0),
	m_triCount(0)
{
}

myMeshLoaderObj::~myMeshLoaderObj()
{
	delete [] m_verts;
	delete [] m_normals;
	delete [] m_tris;
}

bool myMeshLoaderObj::load(const char* filename)
{
    FILE* file;

    float* verts = 0;
    int vertCount = 0;
    int* tris = 0;
    int triCount = 0;

    if(!(file = fopen(filename, "rb")))
        return false;

    // read verts
    fread(&vertCount, sizeof(int), 1, file);
    verts = new float[vertCount*3];
    fread(verts, sizeof(float), vertCount*3, file);

    // read triangles
    fread(&triCount, sizeof(int), 1, file);
    tris = new int[triCount*3];
    fread(tris, sizeof(int), triCount*3, file);

    fclose(file);

    m_verts = verts;
    m_vertCount = vertCount;
    m_tris = tris;
    m_triCount = triCount;

	m_normals = new float[m_triCount*3];
	for (int i = 0; i < m_triCount*3; i += 3)
	{
		const float* v0 = &m_verts[m_tris[i]*3];
		const float* v1 = &m_verts[m_tris[i+1]*3];
		const float* v2 = &m_verts[m_tris[i+2]*3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = &m_normals[i];
		n[0] = e0[1]*e1[2] - e0[2]*e1[1];
		n[1] = e0[2]*e1[0] - e0[0]*e1[2];
		n[2] = e0[0]*e1[1] - e0[1]*e1[0];
		float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
		if (d > 0)
		{
			d = 1.0f/d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}
	
	strncpy(m_filename, filename, sizeof(m_filename));
	m_filename[sizeof(m_filename)-1] = '\0';
	
	return true;
}
