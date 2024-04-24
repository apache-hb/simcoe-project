#ifndef INDEX_SIZE
#   define INDEX_SIZE(id, name, type)
#endif

INDEX_SIZE(eShort, "short", uint16)
INDEX_SIZE(eLong, "long", uint32)

#undef INDEX_SIZE

#ifndef VERTEX_FLAG
#   define VERTEX_FLAG(id, name, value)
#endif

VERTEX_FLAG(eNone, "none", 0)
VERTEX_FLAG(ePosition, "position", 1 << 0)
VERTEX_FLAG(eNormal, "normal", 1 << 1)
VERTEX_FLAG(eTangent, "tangent", 1 << 2)
VERTEX_FLAG(eTexCoord, "texcoord", 1 << 3)

#undef VERTEX_FLAG
