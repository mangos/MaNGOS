#ifndef ADT_H
#define ADT_H

#define TILESIZE (533.33333f)
#define CHUNKSIZE ((TILESIZE) / 16.0f)
#define UNITSIZE (CHUNKSIZE / 8.0f)

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
class Liquid;
typedef struct {
    float x;
    float y;
    float z;
} svec;

typedef struct {
    double x;
    double y;
    double z;
} vec;

typedef struct{
    vec v[3];
} triangle;

typedef struct{
    float v9[16*8+1][16*8+1];
    float v8[16*8][16*8];
} Cell;

typedef struct{
double v9[9][9];
double v8[8][8];
    uint16 area_id;
} chunk;

class WMO;
class WMOManager;
void fixname(std::string &name);

typedef struct
{
    chunk ch[16][16];
} mcell;

struct MapChunkHeader {
    uint32 flags;
    uint32 ix;
    uint32 iy;
    uint32 nLayers;
    uint32 nDoodadRefs;
    uint32 ofsHeight;
    uint32 ofsNormal;
    uint32 ofsLayer;
    uint32 ofsRefs;
    uint32 ofsAlpha;
    uint32 sizeAlpha;
    uint32 ofsShadow;
    uint32 sizeShadow;
    uint32 areaid;
    uint32 nMapObjRefs;
    uint32 holes;
    uint16 s1;
    uint16 s2;
    uint32 d1;
    uint32 d2;
    uint32 d3;
    uint32 predTex;
    uint32 nEffectDoodad;
    uint32 ofsSndEmitters;
    uint32 nSndEmitters;
    uint32 ofsLiquid;                                       // not use in WotLK
    uint32 sizeLiquid;                                      // not use in WotLK
    float  zpos;
    float  xpos;
    float  ypos;
    uint32 textureId;                                       // new offsColorValues in WotLK
    uint32 props;
    uint32 effectId;
};

typedef struct {
    uint32 offsData1;
    uint32 used;
    uint32 offsData2;
} MH2O_offsData;

typedef struct {
    uint16 flags;
    uint16 type;
    float heightLevel1;
    float heightLevel2;
    byte xOffset;
    byte yOffset;
    byte width;
    byte height;
    uint32 ofsData2a;
    uint32 ofsData2b;
} MH2O_Data1;

class MPQFile;

bool MH2O_presence;
MH2O_offsData *LiqOffsData;
MH2O_Data1 *LiqChunkData1;
float *ChunkLiqHeight, *MapLiqHeight;
char* MapLiqFlag;
uint32 k, m, chunk_num;
void LoadMapChunk(MPQFile &, chunk*);
bool LoadWMO(char* filename);
#endif
