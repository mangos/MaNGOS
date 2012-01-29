
#ifndef _MMAP_DEBUG_H
#define _MMAP_DEBUG_H

#include <math.h>
#include "DetourNavMesh.h"
#include "Recast.h"
#include "ChunkyTriMesh.h"
#include "MeshLoaderObj.h"

//void duReadObjMesh(int mapID, rcInputGeom* geom);
void duReadNavMesh(char* tile, dtNavMesh* &navMesh);
int duReadHeightfield(char* tile, rcHeightfield* &hf);
int duReadCompactHeightfield(char* tile, rcCompactHeightfield* &chf);
int duReadContourSet(char* tile, rcContourSet* &cs);
int duReadPolyMesh(char* tile, rcPolyMesh* &mesh);
int duReadDetailMesh(char* tile, rcPolyMeshDetail* &mesh);

class myMeshLoaderObj
{
private:
	float* m_verts;
	int* m_tris;
	float* m_normals;
	int m_vertCount;
	int m_triCount;
	char m_filename[260];

public:
	myMeshLoaderObj();
	~myMeshLoaderObj();
	
	bool load(const char* fileName);

	inline const float* getVerts() const { return m_verts; }
	inline const float* getNormals() const { return m_normals; }
	inline const int* getTris() const { return m_tris; }
	inline int getVertCount() const { return m_vertCount; }
	inline int getTriCount() const { return m_triCount; }
	inline const char* getFileName() const { return m_filename; }
};

enum NavTerrain
{
    NAV_EMPTY   = 0x00,
    NAV_GROUND  = 0x01,
    NAV_MAGMA   = 0x02,
    NAV_SLIME   = 0x04,
    NAV_WATER   = 0x08,
    NAV_UNUSED1 = 0x10,
    NAV_UNUSED2 = 0x20,
    NAV_UNUSED3 = 0x40,
    NAV_UNUSED4 = 0x80
    // we only have 8 bits
};

struct MmapTileHeader
{
    unsigned int mmapMagic;
    unsigned int dtVersion;
    unsigned int mmapVersion;
    unsigned int size;
    bool usesLiquid : 1;
};

#endif
